#include <iostream>
#include <format>
#include <string>
#include <fstream>
#include <Windows.h>

static void whispergateWiper(const CHAR* self);
static void moveFile(const CHAR* self);
static void createDelProc(const CHAR* self);
static void batchDelete(const CHAR* self);

int main(void) {
    CHAR self[MAX_PATH];

    GetModuleFileNameA(nullptr, self, MAX_PATH);

    MessageBoxA(nullptr, "Este programa será deletado em 5 segundos!", "Aviso", MB_OK);

    // whispergateWiper(self);
    // moveFile(self);
    // createDelProc(self);
    // batchDelete(self);

    return 0;
}

static void whispergateWiper(const CHAR* self) {
    std::string command = std::format("cmd.exe /min /C ping 111.111.111.111 -n 5 -w 10 > Nul & Del /f /q {}", self);

    ShellExecuteA(nullptr, "open", "cmd.exe", command.c_str(), nullptr, SW_HIDE);
}

static void moveFile(const CHAR* self) {
    Sleep(5000);
    MoveFileExA(self, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
}

static void batchDelete(const CHAR* self) {
    std::string batFile = std::string(self) + ".bat";
    std::ofstream bat(batFile);

    bat << "@echo off\n"
        << "timeout /t 5 >nul\n"
        << "del \"" << self << "\"\n"
        << "del \"" << batFile << "\"\n";

    bat.close();

    ShellExecuteA(nullptr, "open", batFile.c_str(), nullptr, nullptr, SW_HIDE);

}

static void createDelProc(const CHAR* self) {
    char cmdBuffer[MAX_PATH];

    STARTUPINFOA info;
    PROCESS_INFORMATION procInfo;

    ZeroMemory(&info, sizeof(info));
    ZeroMemory(&procInfo, sizeof(procInfo));
    info.cb = sizeof(info);

    std::string command = std::format("cmd /C timeout /t 5 && del {}", self);
    strcpy_s(cmdBuffer, command.c_str());


    if (CreateProcessA(nullptr, cmdBuffer, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info, &procInfo)) {
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
    }
}