# RequestTrace UAC Bypass

## ⚠️ Important Notice

**This UAC bypass technique only works on Windows 11 24H2 (Build 26100+)**

The exploit automatically detects your Windows version on execution.

## Credits

This is a modernized C++ implementation based on the original work by **R41N3RZUF477**:
- Original Repository: [https://github.com/R41N3RZUF477/RequestTrace_UAC_Bypass](https://github.com/R41N3RZUF477/RequestTrace_UAC_Bypass)
- The payload DLL (`startcmd`) is taken directly from the original repository
- This version refactors the code into modern C++ with namespaces and improved structure

**All credit for discovering and implementing this UAC bypass technique goes to R41N3RZUF477.**

## Overview

This is a User Account Control (UAC) bypass technique that exploits the Windows RequestTrace scheduled task to execute arbitrary DLLs with elevated privileges without triggering a UAC prompt. The technique leverages directory junctions, environment variable manipulation, and a scheduled task triggered by a specific key combination.

## Technical Background

### The RequestTrace Task

Windows includes a scheduled task called `RequestTrace` that can be triggered using the keyboard shortcut **Shift+Ctrl+Win+T**. This task is designed to run with elevated privileges and loads performance tracing handlers from a specific location.

The task searches for and loads `PerformanceTraceHandler.dll` from the following path:
```
%SystemRoot%\System32\PerformanceTraceHandler.dll
```

### The Vulnerability

The exploitation relies on three key components:

1. **Environment Variable Hijacking**: The `%SystemRoot%` environment variable can be manipulated in the user's volatile registry hive (`HKEY_CURRENT_USER\Volatile Environment`)
2. **Directory Junctions**: NTFS junction points can redirect directory paths without requiring elevated privileges
3. **DLL Search Order**: The RequestTrace task will load the DLL from the hijacked path when triggered

## Attack Flow

### Phase 1: Environment Preparation

1. **Create Directory Junction**
   - Create a junction point from `%TEMP%\Registration` → `C:\Windows\Registration`
   - This is necessary because the RequestTrace task validates the Windows registration database
   - Uses `FSCTL_SET_REPARSE_POINT` via `DeviceIoControl` to create a mount point

2. **Deploy Malicious DLL**
   - Copy the payload DLL to `%TEMP%\System32\PerformanceTraceHandler.dll`
   - The DLL structure follows the expected naming convention

3. **Registry Manipulation**
   - Set `HKCU\Volatile Environment\SystemRoot` to point to `%TEMP%`
   - This causes the RequestTrace task to resolve `%SystemRoot%` to the temporary directory

### Phase 2: Trigger Execution

1. **Terminate taskhostw.exe**
   - Kill any non-elevated `taskhostw.exe` processes
   - This ensures a fresh instance will be spawned with the new environment

2. **Send Keyboard Input**
   - Simulate **Shift+Ctrl+Win+T** keypress using `SendInput` API
   - This triggers the RequestTrace scheduled task

3. **Wait for Execution**
   - The task loads from the hijacked `%SystemRoot%` path
   - The malicious DLL executes with High/System integrity

### Phase 3: Cleanup

1. Delete the registry value from `HKCU\Volatile Environment\SystemRoot`
2. Remove the payload DLL from `%TEMP%\System32\`
3. Remove the System32 directory
4. Remove the Registration junction point

## Code Architecture

### Namespaces

#### `DirectoryHandling`
- **`createRegistrationJunction()`**: Creates an NTFS junction point using reparse points
  - Constructs a `REPARSE_DATA_BUFFER` structure
  - Uses `IO_REPARSE_TAG_MOUNT_POINT` tag
  - Critical buffer sizing: `sizeof(REPARSE_DATA_BUFFER) + 14 + sizeof(WCHAR) * MAX_PATH`
  - The `+14` and `+10` bytes are alignment requirements for the reparse point structure
  
- **`deleteRegistrationJunction()`**: Removes the junction point directory

#### `ProcessHandling`
- **`checkProcessElevated()`**: Checks if a process has elevated token using `TokenElevation`
- **`TerminateProcessByPID()`**: Terminates a process by PID, optionally filtering elevated processes
- **`killTaskHostProcesses()`**: Enumerates and terminates taskhostw.exe processes using `CreateToolhelp32Snapshot`

#### `RegistryHandling`
- **`setEnvironmentVariableInRegistry()`**: Sets `SystemRoot` in `HKCU\Volatile Environment`
- **`deleteEnvironmentVariableFromRegistry()`**: Removes the hijacked environment variable

#### `DLLHandling`
- **`copyPayloadDLL()`**: Orchestrates the setup - creates junction, directory, and copies DLL
- **`deletePayloadDLL()`**: Removes all artifacts in reverse order

### Payload DLL (startcmd)

The included payload DLL (`dllmain.c`) demonstrates a simple proof-of-concept:
- Executes `cmd.exe` with elevated privileges on `DLL_PROCESS_ATTACH`
- Calls `ExitProcess(0)` to terminate the host process cleanly
- Can be replaced with any malicious payload following the DllMain convention

## Technical Details

### Reparse Point Buffer Structure

The critical aspect of creating junction points is the precise buffer sizing:

```cpp
// Buffer allocation
constexpr size_t bufferSize = sizeof(REPARSE_DATA_BUFFER) + 14 + sizeof(WCHAR) * MAX_PATH;

// Path size calculation
int regdbPathLen = wRegdbPath.length() * sizeof(WCHAR) + sizeof(WCHAR); // Include null terminator
ULONG rdbPathSize = regdbPathLen + 10; // Add alignment bytes
ULONG rdbSize = REPARSE_DATA_BUFFER_HEADER_LENGTH + rdbPathSize;

// SubstituteNameLength excludes null terminator
reparseData->MountPointReparseBuffer.SubstituteNameLength = regdbPathLen - sizeof(WCHAR);
```

The `+14` and `+10` byte offsets are crucial - without them, Windows rejects the reparse point with `ERROR_PRIVILEGE_NOT_HELD` (0x1128), which is misleading as the actual issue is malformed structure rather than insufficient privileges.

### Key Combination Simulation

The attack uses `SendInput` to simulate the Shift+Ctrl+Win+T keyboard shortcut:

```cpp
INPUT inputs[8];
// Key down: Shift, Ctrl, Win, T
// Key up: T, Win, Ctrl, Shift (reverse order)
SendInput(8, &inputs[0], sizeof(INPUT));
```

## Usage

### Building

1. **Main Executable** (Visual Studio):
   ```
   Configuration: Release x64
   Platform Toolset: v143 or compatible
   ```

2. **Payload DLL** (startcmd):
   ```
   Configuration: Release x64
   Output: startcmd.dll
   ```

### Execution

```cmd
RequestTrace_UAC_Bypass.exe <path_to_payload.dll>
```

Example:
```cmd
RequestTrace_UAC_Bypass.exe C:\temp\startcmd.dll
```

### Workflow

1. The executable sets up the environment (junction, DLL copy, registry)
2. Simulates Shift+Ctrl+Win+T keypress
3. RequestTrace task executes with elevated privileges
4. Payload DLL loads and executes (e.g., spawns elevated cmd.exe)
5. Cleanup removes all artifacts

## Detection & Mitigation

### Indicators of Compromise (IOCs)

1. **Registry**:
   - Creation of `HKCU\Volatile Environment\SystemRoot` (normally doesn't exist)
   
2. **File System**:
   - Unusual junction points in user-writable directories
   - `PerformanceTraceHandler.dll` in unexpected locations (e.g., `%TEMP%\System32\`)

3. **Process Behavior**:
   - Abnormal termination of `taskhostw.exe` processes
   - Unusual keyboard input simulation (Shift+Ctrl+Win+T)
   - RequestTrace task loading DLLs from non-standard paths

### Mitigations

1. **Group Policy**: Disable the RequestTrace scheduled task if not required
2. **Monitoring**: Alert on modifications to `HKCU\Volatile Environment`
3. **EDR**: Monitor for junction point creation in user-writable directories
4. **Application Whitelisting**: Restrict DLL loading paths for scheduled tasks
5. **Sysmon**: Track Event ID 1 (Process Creation) with elevated tokens from unexpected parents

## References

- [REPARSE_DATA_BUFFER Structure](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_reparse_data_buffer)
- [FSCTL_SET_REPARSE_POINT](https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/fsctl-set-reparse-point)
- [UAC Bypass Techniques](https://github.com/hfiref0x/UACME)

## Disclaimer

This code is provided for **educational and research purposes only**. Unauthorized use of this technique against systems you do not own or have explicit permission to test is illegal. The authors are not responsible for any misuse or damage caused by this software.

## License

This project is for educational purposes. Use responsibly and ethically.
