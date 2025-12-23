# Heaven's Gate CTF

A pure x86 Assembly implementation demonstrating the **Heaven's Gate** technique, transitioning from 32-bit to 64-bit using code segment selector.

## Overview

This project showcases the Heaven's Gate technique, which allows a 32-bit (WoW64) process to execute 64-bit code by manipulating the code segment selector. The key verification logic runs entirely in 64-bit mode, making it significantly harder to analyze with standard 32-bit debuggers.

## How It Works

1. **32-bit Initialization**: The program starts in 32-bit mode, gets console handles, and reads user input
2. **Mode Transition**: Uses `push 0x33; push addr; retf` to switch to 64-bit mode (CS=0x33)
3. **64-bit Key Verification**: Performs obfuscated string comparisons in 64-bit mode with anti-debug checks
4. **Return to 32-bit**: Uses `push 0x23; push addr; retfq` to return to 32-bit mode (CS=0x23)
5. **Result Display**: Shows success or failure message based on verification result

## Technical Details

### Key Components

- **Segment Selectors**: Uses CS=0x33 for 64-bit mode and CS=0x23 for 32-bit mode on WoW64
- **Stack Alignment**: Aligns stack to 8 bytes before transitioning (`and esp, 0xFFFFFFF8`)
- **REX.W Prefix**: Uses `db 0x48` before `retf` for proper 64-bit far return
- **Anti-Debug**: Checks `BeingDebugged` flag in PEB via `GS:[0x60]`

## Building

### Requirements

- NASM (Netwide Assembler)
-  MSVC link.exe or MinGW-w64 or GoLink (32-bit linker)
- Windows OS (WoW64 environment)

### Build Instructions

**Using NASM + MSVC link:**
```batch
nasm -f win32 heavens_gate.asm -o heavens_gate.o
link.exe heavens_gate.o kernel32.lib /ENTRY:main /OUT:heavens_gate.exe
```

**Using NASM + GCC (MinGW):**
```batch
nasm -f win32 heavens_gate.asm -o heavens_gate.o
gcc -m32 heavens_gate.o -o heavens_gate.exe -nostdlib -lkernel32
```

**Using NASM + GoLink:**
```batch
nasm -f win32 heavens_gate.asm -o heavens_gate.o
golink /entry:main heavens_gate.o kernel32.dll
```

## Usage

Run the compiled executable:

```batch
.\heavens_gate.exe
```

The program will:
1. Display a prompt asking for the key
2. Read user input
3. Transition to 64-bit mode (displays "!!!Made in Heaven!!!")
4. Verify the key using obfuscated comparisons
5. Check for debugger presence
6. Return to 32-bit mode and display result

## Why Heaven's Gate?

The Heaven's Gate technique is historically significant in malware development because:

- **Debugger Evasion**: Most 32-bit debuggers cannot follow the transition to 64-bit code
- **Static Analysis Bypass**: Disassemblers may misinterpret 64-bit instructions in a 32-bit binary
- **AV/EDR Evasion**: Some security tools don't properly handle WoW64 transitions
- **API Hooking Bypass**: Can call 64-bit ntdll directly, bypassing 32-bit hooks

## References and Credits

### 1. Desafios de Engenharia Reversa (hackingnaweb)
- **Resource** [Desafios de Engenharia Reversa](https://hackingnaweb.com/desafios/desafio9.zip)
- **Description**: A CTF zip created by hackingnaweb that explores the technique. The zip password is: hackingnaweb.com

### 2. WoW64 Internals
- **Resource**: [Microsoft WoW64 Implementation Details](https://docs.microsoft.com/en-us/windows/win32/winprog64/running-32-bit-applications)
- **Description**: Official documentation on Windows 32-bit emulation layer

### 3. Practical Implementation
- **Resource**: [JustasMasiulis/wow64pp](https://github.com/JustasMasiulis/wow64pp)
- **Description**: Modern C++ library for Heaven's Gate technique

## Educational Purpose

> ⚠️ **DISCLAIMER**: This project is for educational and CTF/research purposes only. It demonstrates security research concepts and should only be used in controlled environments with proper authorization. Unauthorized use of these techniques may violate laws and regulations.

## Detection and Mitigation

### Detection Strategies
- Monitor for unusual segment selector changes (CS=0x33 from 32-bit process)
- Detect `retf` instructions with 64-bit segment selectors
- Behavioral analysis of WoW64 transitions
- ETW tracing for cross-architecture calls

### Mitigation Approaches
- Use 64-bit security tools that can handle both modes
- Monitor debug register and segment selector modifications
- Implement kernel-level hooks that cover both architectures

## License

This project is provided as-is for educational purposes. Use responsibly and ethically.


---

**Last Updated:** December 2025
