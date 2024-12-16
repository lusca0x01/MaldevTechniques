#define _WIN32_DCOM

#include <iostream>
#include <intrin.h>
#include <algorithm>
#include <Windows.h>
#include <TlHelp32.h>
#include <comdef.h>
#include <Wbemidl.h>

#include "tokens.h"

#pragma comment(lib, "wbemuuid.lib")

int main(void) {
    std::tuple<IWbemServices*, IWbemLocator*> wql = init_wql();

    bool result = check_registry() | check_registry_keys() | check_fs_artifacts()
        | check_running_procs() | check_vm_vendor() | has_hypervisor()
        | query_msvm(std::get<0>(wql), std::get<1>(wql));

    if (result) {
        std::cout << "Running in a VM :(" << std::endl;
        return 1;
    }

    std::cout << "Free to hack it :)" << std::endl;
    return 0;
}

bool check_registry() {
    std::vector<std::string> found_values;

    for (const std::tuple<HKEY, std::string, std::string, std::string>& key : registry_keys_value_artifacts) {
        DWORD dataType;
        BYTE value[255];
        DWORD valueSize = sizeof(value);
        HKEY hKey;

        if (RegOpenKeyA(std::get<0>(key), std::get<1>(key).c_str(), &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, std::get<2>(key).c_str(), nullptr, &dataType, value, &valueSize) == ERROR_SUCCESS) {
                if (dataType == REG_SZ) {
                    value[valueSize - 1] = '\0';
                    std::string value_str(reinterpret_cast<LPCSTR>(value));
                    if (value_str == std::get<3>(key)) {
                        std::cout << "(Check Registry Values) Got you: " << value_str << std::endl;
                        found_values.push_back(value_str);
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }

    return !found_values.empty();
}

bool check_registry_keys() {
    std::vector<std::string> found_keys;

    for (const std::tuple<HKEY, std::string>& key : registry_keys_artifacts) {
        HKEY result;
        std::string key_str = std::get<1>(key);
        if (RegOpenKeyA(std::get<0>(key), key_str.c_str(), &result) == ERROR_SUCCESS) {
            found_keys.push_back(key_str);
            std::cout << "(Check Registry Keys) Got you: " << key_str << std::endl;
            RegCloseKey(result);
        }
    }

    return !found_keys.empty();
}

bool check_fs_artifacts() {
    std::vector<std::string> found_artifacts;

    for (const std::string& file : file_system_artifacts) {

        // Check if file exists in the system
        DWORD attributes = GetFileAttributesA(file.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            found_artifacts.push_back(file);
            std::cout << "(Check Files) Found you: " << file << std::endl;
        }
    }

    return !found_artifacts.empty();
}

bool check_running_procs() {
    std::vector<std::wstring> found_procs;

    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcSnap == INVALID_HANDLE_VALUE) {
        std::cout << "Error taking process snapshot: " << GetLastError() << std::endl;
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    std::vector<std::wstring> target_wprocesses;
    for (const std::string& proc : target_processes) {
        target_wprocesses.push_back(std::wstring(proc.begin(), proc.end()));
    }

    if (Process32First(hProcSnap, &pe32)) {
        do {
            bool result = std::any_of(target_wprocesses.begin(), target_wprocesses.end(),
                [&pe32](const std::wstring& value) { return value == pe32.szExeFile; });

            if (result) {
                found_procs.push_back(pe32.szExeFile);
                std::wcout << "(Check Processes) Found a proc: " << pe32.szExeFile << std::endl;
            }
        } while (Process32Next(hProcSnap, &pe32));
    }

    CloseHandle(hProcSnap);
    return !found_procs.empty();
}

bool check_vm_vendor() {
    int cpuInfo[4] = { 0 };
    char vendor[13] = { 0 };

    __cpuid(cpuInfo, 0);

    *reinterpret_cast<int*>(vendor) = cpuInfo[1];  // EBX
    *reinterpret_cast<int*>(vendor + 4) = cpuInfo[2]; // ECX
    *reinterpret_cast<int*>(vendor + 8) = cpuInfo[3]; // EDX
    vendor[12] = '\0';

    const std::string vendorStr(vendor);

    // Check if the vendor is in the vendor_artifacts list
    bool result = std::any_of(vendor_artifacts.begin(), vendor_artifacts.end(),
        [&vendorStr](const std::string& value) { return vendorStr == value; });

    if (result) {
        std::cout << "(Check Vendor) Found a vendor: " << vendorStr << std::endl;
    }

    return result;
}

bool has_hypervisor() {
    int cpuInfo[4] = { 0 };

    __cpuid(cpuInfo, 0x40000000);

    if (cpuInfo[0] == 0 && cpuInfo[1] == 0 && cpuInfo[2] == 0 && cpuInfo[3] == 0) {
        return false;
    }

    char vendor[13] = { 0 };
    *reinterpret_cast<int*>(vendor) = cpuInfo[1];  // EBX
    *reinterpret_cast<int*>(vendor + 4) = cpuInfo[2]; // ECX
    *reinterpret_cast<int*>(vendor + 8) = cpuInfo[3]; // EDX

    const std::string vendorStr(vendor);

    // For some reason, on bare-metal, it returns Microsoft Hyper-V
    if (vendorStr == "Microsoft Hv") {
        std::cout << "(MAYBE!) ON Microsoft Hyper-V" << std::endl;
        return false;
    }

    std::cout << "Hypervisor vendor: " << vendor << std::endl;

    return true;
}

void check_hresult(HRESULT hres, const std::string& message) {
    if (FAILED(hres)) {
        std::cout << message << " Error code = 0x" << std::hex << hres << std::endl;
        throw std::runtime_error(message);
    }
}

std::tuple<IWbemServices*, IWbemLocator*> init_wql() {
    check_hresult(CoInitializeEx(0, COINIT_MULTITHREADED), "Failed to initialize COM library.");
    check_hresult(CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    ), "Failed to initialize security.");

    IWbemLocator* pLoc = nullptr;
    check_hresult(CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    ), "Failed to create IWbemLocator object.");

    IWbemServices* pSvc = nullptr;
    check_hresult(pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    ), "Could not connect to WMI namespace.");

    check_hresult(CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    ), "Could not set proxy blanket.");

    return { pSvc, pLoc };
}


bool query_msvm(IWbemServices* pSvc, IWbemLocator* pLoc) {
    bool result = false;
    HRESULT hres;

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Msvm_SummaryInformation"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        std::cout << "Query for operating system name failed."
            << " Error code = 0x"
            << std::hex << hres << std::endl;
        cleanup_wql(pSvc, pLoc);
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(
            WBEM_INFINITE,
            1,
            &pclsObj,
            &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        VariantInit(&vtProp);
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        if (vtProp.vt != VT_NULL) {
            std::wcout << "(WMI Msvm_SummaryInformation QUERY) VM GUID: " << vtProp.bstrVal << std::endl;
            result = true;
        }
        VariantClear(&vtProp);
        hr = pclsObj->Get(L"ElementName", 0, &vtProp, 0, 0);
        if (vtProp.vt != VT_NULL) {
            std::wcout << "(WMI Msvm_SummaryInformation QUERY) VM Name: " << vtProp.bstrVal << std::endl;
            result = true;
        }
        VariantClear(&vtProp);
        hr = pclsObj->Get(L"EnabledState", 0, &vtProp, 0, 0);
        if (vtProp.vt != VT_NULL) {
            std::wcout << "(WMI Msvm_SummaryInformation QUERY) EnabledState: " << vtProp.ulVal << std::endl;
            result = true;
        }
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    pEnumerator->Release();
    cleanup_wql(pSvc, pLoc);
    return result;
}

void cleanup_wql(IWbemServices* pSvc, IWbemLocator* pLoc) {
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    CoUninitialize();
}