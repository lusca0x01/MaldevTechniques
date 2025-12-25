# AMSI Bypass Using Hardware Breakpoints

A modern C++ implementation of an AMSI (Antimalware Scan Interface) bypass technique using hardware breakpoints and vectored exception handlers.

## Overview

This project demonstrates how to bypass Windows AMSI by leveraging CPU hardware breakpoints (debug registers) to intercept and manipulate the `AmsiScanBuffer` function's behavior. When AMSI attempts to scan content, the hardware breakpoint triggers an exception that is handled by a custom vectored exception handler, which modifies the scan result to indicate clean content.

## How It Works

1. **Hardware Breakpoint Setup**: The technique sets a hardware breakpoint (DR0) on the `AmsiScanBuffer` function address
2. **Exception Handling**: A vectored exception handler intercepts the `EXCEPTION_SINGLE_STEP` exception when `AmsiScanBuffer` is called
3. **Result Manipulation**: The handler modifies the 6th argument (AMSI_RESULT* result) to return `AMSI_RESULT_CLEAN`
4. **Execution Flow Control**: The handler adjusts the instruction pointer to skip the actual AMSI scan and return immediately

## Technical Details

### Key Components

- **NT API Functions**: Uses `NtGetContextThread` and `NtSetContextThread` for thread context manipulation
- **Debug Registers**: Utilizes DR0-DR3 (breakpoint addresses) and DR7 (control register) for hardware breakpoint configuration
- **Vectored Exception Handler**: Custom exception handler registered with `AddVectoredExceptionHandler` to catch single-step exceptions
- **x64 Calling Convention**: Correctly handles Windows x64 calling convention (RCX, RDX, R8, R9, then stack) to access function arguments

### Workflow

```
AmsiScanBuffer called
    ↓
Hardware breakpoint triggers (DR0)
    ↓
EXCEPTION_SINGLE_STEP raised
    ↓
VectoredExceptionHandler invoked
    ↓
Modify AMSI_RESULT to AMSI_RESULT_CLEAN
    ↓
Adjust RIP to return address
    ↓
Clear hardware breakpoint
    ↓
Resume execution (AMSI bypassed)
```

## Building

### Requirements

- Visual Studio 2019 or later
- Windows SDK
- C++17 or later

### Build Instructions

1. Open `Amsi Bypass Hardware Breakpoints.sln` in Visual Studio
2. Select **Release** configuration and **x64** platform
3. Build the solution (Ctrl+Shift+B)
4. The executable will be generated in `x64/Release/`

## Usage

Simply run the compiled executable:

```powershell
.\x64\Release\"Amsi Bypass Hardware Breakpoints.exe"
```

The program will:
1. Initialize AMSI context
2. Test AMSI detection with EICAR string BEFORE bypass
3. Setup hardware breakpoint on `AmsiScanBuffer`
4. Test AMSI detection with EICAR string AFTER bypass
5. Display results indicating whether the bypass was successful

### About the EICAR Test String

The program uses the **EICAR (European Institute for Computer Antivirus Research) Standard Anti-Virus Test File** to validate the AMSI bypass. The EICAR test string is a standardized, harmless text file that has been adopted by antivirus vendors as a way to test anti-malware software without using actual malicious code.

The EICAR string used in this project:
```
X5O!P%@AP[4\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*
```

When AMSI is functioning normally, it will flag this string as malicious. However, when the hardware breakpoint bypass is active, AMSI will report it as clean, demonstrating the effectiveness of the bypass technique.

**Reference**: [EICAR Test File Official Website](https://www.eicar.org/download-anti-malware-testfile/)

### Expected Output

```
Testing AMSI bypass...

Scanning EICAR string BEFORE bypass...
AMSI detected the string as malicious before bypass. Normal flow
Setting up AMSI bypass...
AmsiScanBuffer found at: 0x<address>
Exception handler registered
Hardware breakpoint set on AmsiScanBuffer

Scanning EICAR string AFTER bypass...
AMSI Bypass invoked at address: 0x<address>
Original AMSI result: 32768
Modified AMSI result to: 0
AMSI bypassed, the string was not detected as malicious.

Bypass successful!
```

## Code Structure

```cpp
main.cpp
├── GlobalState namespace         // Global variables and constants
├── ContextHelper namespace       // Thread context manipulation helpers
├── HardwareBreakpoint namespace  // Debug register management
├── VectoredExceptionHandler()    // Custom exception handler
├── AmsiContext class            // RAII wrapper for AMSI operations
├── SetupAmsiBypass()            // Hardware breakpoint setup
├── TestAmsiBypass()             // Bypass testing logic
└── main()                       // Entry point
```

## References and Credits

This implementation is inspired by and references the following projects:

### 1. Rust Implementation
**Rust for Malware Development - AMSI Bypass with Hardware Breakpoints**
- Repository: [Whitecat18/Rust-for-Malware-Development](https://github.com/Whitecat18/Rust-for-Malware-Development/tree/main/AMSI%20BYPASS/Amsi_HBP)
- Language: Rust
- A Rust-based implementation of the same technique using hardware breakpoints to bypass AMSI

### 2. PowerShell Implementation
**S3cur3Th1sSh1t's AMSI Bypass Collection - Hardware Breakpoints**
- Repository: [S3cur3Th1sSh1t/Amsi-Bypass-Powershell](https://github.com/S3cur3Th1sSh1t/Amsi-Bypass-Powershell?tab=readme-ov-file#Using-Hardware-Breakpoints)
- Language: PowerShell
- Documentation and PowerShell implementation of various AMSI bypass techniques including the hardware breakpoint method

## Educational Purpose

⚠️ **DISCLAIMER**: This project is for **educational and research purposes only**. It demonstrates security research concepts and should only be used in controlled environments with proper authorization. Unauthorized use of these techniques may violate laws and regulations.

## Detection and Mitigation

### Detection Strategies
- Monitor for calls to `AddVectoredExceptionHandler` from suspicious processes
- Detect unusual hardware breakpoint usage via debug register monitoring
- Behavioral analysis of AMSI scan result patterns (unusually high clean rates)
- ETW (Event Tracing for Windows) monitoring for AMSI-related events

### Mitigation Approaches
- Kernel-level AMSI implementation
- Protected Process Light (PPL) for AMSI provider
- Monitoring debug register modifications
- Enhanced logging and alerting on AMSI bypass attempts

## License

This project is provided as-is for educational purposes. Use responsibly and ethically.

## Contributing

Contributions, suggestions, and improvements are welcome. Please ensure any modifications maintain the educational focus of this project.

---

**Last Updated**: December 2025
