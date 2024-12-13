#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ntdll.lib")

#define SE_SHUTDOWN_PRIVILEGE 19
#define OptionShutdownSystem 6
#define STATUS_FWP_NULL_POINTER 0xC022001C // Just an eror status example

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(
	ULONG Privilege,
	BOOLEAN Enable,
	BOOLEAN CurrentThread,
	PBOOLEAN Enabled
);

extern "C" NTSTATUS NTAPI NtRaiseHardError(
	NTSTATUS ErrorStatus,
	ULONG NumberOfParameters,
	ULONG UnicodeStringParameterMask,
	PVOID Parameters,
	ULONG ResponseOption,
	PULONG ResponsePointer
);


int main() {
	BOOLEAN wasEnabled;
	ULONG responsePointer;

	RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, true, FALSE, &wasEnabled);
	NtRaiseHardError(STATUS_FWP_NULL_POINTER, 0, 0, 0, OptionShutdownSystem, &responsePointer);
}