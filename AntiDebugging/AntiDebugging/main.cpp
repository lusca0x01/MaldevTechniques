#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ntdll.lib")

extern "C" NTSTATUS NTAPI NtQueryInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
);

bool time_checker();
bool flags_checker();
bool exceptions_checker();
bool windows_and_processes_checker();

int main(void) {
	time_checker();

	bool result = flags_checker() | windows_and_processes_checker();

	// result = result | exceptions_checker();

	if (result) {
		std::cout << "Debugger is present";
	}
	else {
		std::cout << "No debugger here";
	}


	return 0;
}

bool flags_checker() {
	bool result = false;

	if (IsDebuggerPresent()) {
		std::cout << "Debugger spotted by IsDebuggerPresent" << std::endl;
		result = true;
	}

	BOOL pbDebuggerPresent;
	if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &pbDebuggerPresent) == TRUE && pbDebuggerPresent == TRUE) {
		std::cout << "Debugger spotted by CheckRemoteDebuggerPresent" << std::endl;
		result = true;
	}

	// Debugger port number
	DWORD dwProcessDebugPort, dwReturned;
	NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort, &dwProcessDebugPort, sizeof(DWORD), &dwReturned);

	if (NT_SUCCESS(status) && dwProcessDebugPort == -1) {
		std::cout << "Debugger spotted by NtQueryInformationProcess (port number)" << std::endl;
		result = true;
	}

	// EPROCESS -> NoDebugInherit
	DWORD dwProcessDebugFlags;
	const DWORD ProcessDebugFlags = 0x1f;
	status = NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)ProcessDebugFlags, &dwProcessDebugFlags, sizeof(DWORD), &dwReturned);

	if (NT_SUCCESS(status) && dwProcessDebugFlags == 0) {
		std::cout << "Debugger spotted by NtQueryInformationProcess (NoDebugInherit)" << std::endl;
		result = true;
	}

	// Debug object
	HANDLE hProcessDebugObject = 0;
	const DWORD ProcessDebugObjectHandle = 0x1e;
	status = NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)ProcessDebugObjectHandle, hProcessDebugObject, sizeof(HANDLE), &dwReturned);

	if (NT_SUCCESS(status) && hProcessDebugObject != 0) {
		std::cout << "Debugger spotted by NtQueryInformationProcess (Debug Object)" << std::endl;
		result = true;
	}
	
	return result;
}

// When I was testing, the process printed it in release mode. It is inconsistent.
bool exceptions_checker() {
	bool result = false;

	__try {
		RaiseException(DBG_CONTROL_C, 0, 0, nullptr);
		return true;
	}
	__except (DBG_CONTROL_C == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		std::cout << "Debugger spotted by RaiseException(DBG_CONTROL_C)" << std::endl;
		result = true;
	}

	__try {
		RaiseException(DBG_PRINTEXCEPTION_C, 0, 0, nullptr);
	}
	__except (DBG_PRINTEXCEPTION_C == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		std::cout << "Debugger spotted by RaiseException(DBG_PRINTEXCEPTION_C)" << std::endl;
		result = true;
	}

	return result;
}

bool windows_and_processes_checker() {
	bool result = false;

	const std::vector<std::string> debuggers_windows = {
		"antidbg",
		"ID",
		"ntdll.dll",
		"ObsidianGUI",
		"OLLYDBG",
		"Rock Debugger",
		"SunAwtFrame",
		"Qt5QWindowIcon"
		"WinDbgFrameClass",
		"Zeta Debugger",
		"x64dbg",
		"x32dbg"
	};

	for (const std::string& debugger : debuggers_windows) {
		if (FindWindowA(nullptr, debugger.c_str()) != nullptr) {
			std::cout << "Debugger Window spotted: " << debugger << std::endl;
			result = true;
		}
	}

	HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcSnap == INVALID_HANDLE_VALUE) {
		std::cout << "Error taking process snapshot: " << GetLastError() << std::endl;
		return 1;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	std::vector<std::wstring> debuggers_procs;
	for (const std::string& proc : debuggers_windows) {
		debuggers_procs.push_back(std::wstring(proc.begin(), proc.end()) + L".exe");
	}

	if (Process32First(hProcSnap, &pe32)) {
		do {
			bool is_one = std::any_of(debuggers_procs.begin(), debuggers_procs.end(),
				[&pe32](const std::wstring& value) { return value == pe32.szExeFile; });
			if (is_one) { 
				std::wcout << "Debugger Process spotted: " << pe32.szExeFile << std::endl;
				result = true; 
			}
		} while (Process32Next(hProcSnap, &pe32));
	}
	CloseHandle(hProcSnap);

	return result;
}

bool time_checker() {
	bool result = false;
	bool is_bigger = false;

	DWORD start = GetTickCount();
	// Put a code that perform some operation in here and evaluate the time cost.
	DWORD end = GetTickCount();

	is_bigger = (end - start) > 1;

	if (is_bigger) { 
		std::cout << "Debugger spotted by GetTickCount" << std::endl;
		result = is_bigger;
	}

	SYSTEMTIME sys_time_start, sys_time_end;
	FILETIME file_time_start, file_time_end;
	ULARGE_INTEGER uli_start, uli_end;

	GetLocalTime(&sys_time_start);
	// Put a code that perform some operation in here and evaluate the time cost.
	GetLocalTime(&sys_time_end);

	if (SystemTimeToFileTime(&sys_time_start, &file_time_start) && SystemTimeToFileTime(&sys_time_end, &file_time_end)) {
		uli_start.LowPart = file_time_start.dwLowDateTime;
		uli_start.HighPart = file_time_start.dwHighDateTime;
		uli_end.LowPart = file_time_end.dwLowDateTime;
		uli_end.HighPart = file_time_end.dwHighDateTime;

		is_bigger = (uli_end.QuadPart - uli_start.QuadPart) > 1;

		if (is_bigger) {
			std::cout << "Debugger spotted by GetLocalTime" << std::endl;
			result = is_bigger;
		}
	}

	GetSystemTime(&sys_time_start);
	// Put a code that perform some operation in here and evaluate the time cost.
	GetSystemTime(&sys_time_end);

	if (SystemTimeToFileTime(&sys_time_start, &file_time_start) && SystemTimeToFileTime(&sys_time_end, &file_time_end)) {
		uli_start.LowPart = file_time_start.dwLowDateTime;
		uli_start.HighPart = file_time_start.dwHighDateTime;
		uli_end.LowPart = file_time_end.dwLowDateTime;
		uli_end.HighPart = file_time_end.dwHighDateTime;

		is_bigger = (uli_end.QuadPart - uli_start.QuadPart) > 1;

		if (is_bigger) {
			std::cout << "Debugger spotted by GetLocalTime" << std::endl;
			result = is_bigger;
		}
	}

	LARGE_INTEGER li_start, li_end;
	QueryPerformanceCounter(&li_start);
	// Put a code that perform some operation in here and evaluate the time cost.
	QueryPerformanceCounter(&li_end);
	is_bigger = (li_end.QuadPart - li_start.QuadPart) > 1;

	if (is_bigger) {
		std::cout << "Debugger spotted by QueryPerformanceCounter" << std::endl;
		result = is_bigger;
	}


	return result;
}