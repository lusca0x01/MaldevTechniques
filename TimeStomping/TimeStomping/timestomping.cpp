#include <iostream>
#include "timestomping.h"
#include <windows.h>

// Link NTDLL
#pragma comment(lib, "ntdll.lib")

extern "C" NTSTATUS NTAPI NtQueryInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

extern "C" NTSTATUS NTAPI NtSetInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

int main() {
    FILE_BASIC_INFORMATION fileInfo = { 0 };
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    HANDLE file = CreateFileA("test.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create file. Error: " << GetLastError() << "\n";
        return 1;
    }

    // Query file information
    NTSTATUS status = NtQueryInformationFile(
        file,
        &ioStatusBlock,
        &fileInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
    );

    if (status < 0) {
        std::cerr << "Failed to query file information. NTSTATUS: 0x" << std::hex << status << "\n";
        CloseHandle(file);
        return 1;
    }

    std::cout << "Original Creation Time: " << fileInfo.CreationTime.QuadPart << "\n";
    std::cout << "Original Last Access Time: " << fileInfo.LastAccessTime.QuadPart << "\n";

    fileInfo.CreationTime.QuadPart = 1226265600000000; // November 20, 1604, 04:56:00 UTC
    fileInfo.LastAccessTime.QuadPart = 1226265600000000;
    fileInfo.LastWriteTime.QuadPart = 1226265600000000;
    fileInfo.ChangeTime.QuadPart = 1226265600000000;

    status = NtSetInformationFile(
        file,
        &ioStatusBlock,
        &fileInfo,
        sizeof(FILE_BASIC_INFORMATION),
        FileBasicInformation
    );

    if (status < 0) {
        std::cerr << "Failed to set file information. NTSTATUS: 0x" << std::hex << status << "\n";
        CloseHandle(file);
        return 1;
    }

    std::cout << "Timestamps updated successfully" << std::endl;

    CloseHandle(file);

    return 0;
}
