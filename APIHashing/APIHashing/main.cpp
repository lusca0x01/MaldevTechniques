#include <Windows.h>
#include <iostream>

#define HASH 1281079708  // The hash value for "ShellExecuteA"

typedef int (WINAPI* PShellExecuteA)(HWND hwnd,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPCSTR lpParameters,
    LPCSTR lpDirectory,
    INT nShowCmd);

int main() {
    // Load Shell32.dll
    HMODULE hModule = LoadLibraryA("Shell32.dll");

    if (!hModule) {
        std::cout << "Failed to load Shell32.dll" << std::endl;
        return 1;
    }

    // Access the DOS and NT headers
    PIMAGE_DOS_HEADER pImgDosHdr = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pImgNtHdr = (PIMAGE_NT_HEADERS)((LPBYTE)hModule + pImgDosHdr->e_lfanew);

    // Get the export directory
    DWORD exportDirRVA = pImgNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportDirSize = pImgNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    PIMAGE_EXPORT_DIRECTORY pExportHdr = (PIMAGE_EXPORT_DIRECTORY)((LPBYTE)hModule + exportDirRVA);

    // Get the addresses of the functions and names
    PDWORD addrFuncs = (PDWORD)((DWORD_PTR)hModule + pExportHdr->AddressOfFunctions);
    PDWORD addrNames = (PDWORD)((DWORD_PTR)hModule + pExportHdr->AddressOfNames);
    PWORD addrNameOrd = (PWORD)((DWORD_PTR)hModule + pExportHdr->AddressOfNameOrdinals);

    // Iterate over the exported functions
    for (int i = 0; i < (pExportHdr->NumberOfNames); ++i) {
        int result = 0;
        char* name = (char*)(((LPBYTE)hModule) + addrNames[i]);

        // Calculate the hash of the function name using the algorithm
        for (int j = 0; name[j] != '\0'; ++j) {
            result = (result * 31) + name[j];
        }

        if (result == HASH) {
            // Get the function address
            DWORD_PTR funcAddrRVA = addrFuncs[addrNameOrd[i]];
            PShellExecuteA pShellExecuteA = (PShellExecuteA)((DWORD_PTR)hModule + funcAddrRVA);

            // Call ShellExecuteA to open calc.exe
            pShellExecuteA(NULL, "open", "calc.exe", NULL, NULL, SW_SHOWNORMAL);
            break;
        }
    }

    FreeLibrary(hModule);
    return 0;
}
