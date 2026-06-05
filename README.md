# Tokenizer - Windows Token Duplication Tool

**A clean, single-file utility to duplicate a target process's token (including many PPL-protected processes) and spawn a high-privilege `cmd.exe`.**

---

## 📋 Description

This tool performs **token duplication** on Windows by:

- Enabling `SeDebugPrivilege`
- Finding a target process by name (e.g. `services.exe`, `lsass.exe`, `winlogon.exe`, etc.)
- Opening the process with minimal rights (`PROCESS_QUERY_LIMITED_INFORMATION`) — works on many Protected Process Light (PPL) processes
- Duplicating the token using `MAXIMUM_ALLOWED` rights and `SecurityDelegation`
- Enabling **every possible privilege** on the duplicated token
- Spawning a new `cmd.exe` (usually running as `NT AUTHORITY\SYSTEM`) in a fresh console window

## Versions Overview

| Version                    | File                        | Technique                                                                                  | Evasion Level | Status      | Recommended |
|---------------------------|-----------------------------|---------------------------------------------------------------------------------------------|---------------|-------------|-------------|
| **Win32 Baseline**        | `main.c`                    | Classic `OpenProcess` → `OpenProcessToken` → `DuplicateTokenEx` → `CreateProcessWithTokenW` | Low           | Baseline    | Reference only |
| **NT Direct**             | `token_nt_direct.c`         | Dynamic `NtOpenProcess` + `NtOpenProcessToken` + `NtDuplicateToken`                         | Medium        | Good        | Useful for comparison |
| **Indirect Handle**       | `token_indirect_handle.c`   | `NtDuplicateObject` + System Handle Table + NT token APIs | High                            | **Best**       | **Current recommended version** |

---

## Version Details

### 1. main.c (Baseline)
- Uses standard Win32 APIs throughout.
- Simple and easy to understand.
- Heavily signatured by most user-mode EDRs.
- Good starting point / reference implementation.

### 2. token_nt_direct.c
- Moves token operations to the NT layer using dynamic resolution from `ntdll.dll`.
- Replaces `OpenProcess`, `OpenProcessToken`, and `DuplicateTokenEx` with their NT equivalents.
- Still opens the target process directly (`NtOpenProcess`).
- Better evasion than pure Win32, but still has direct process access telemetry.

### 3. token_indirect_handle.c (Current Best)
- **Key improvement**: Obtains a handle to the target process **indirectly** using `NtDuplicateObject` + walking the system handle table.
- Uses dynamic NT APIs for all token operations.
- Significantly reduces direct "I opened services.exe" style telemetry.
- Still uses `CreateProcessWithTokenW` for reliable process creation with the duplicated primary token.
- Currently the most evasive and recommended version.


Perfect for security research, red teaming, and learning Windows token manipulation.

---

## ✨ Features

- ✅ Works on many PPL-protected processes
- ✅ Automatically enables **all privileges** on the new token
- ✅ Minimal permissions required to open the target process
- ✅ Clean, well-commented code with good error handling
- ✅ No external dependencies — pure Win32 API
- ✅ Single source file, easy to compile and use

---

## 🛠️ Requirements

- **OS**: Windows 10 / Windows 11 (x64 recommended)
- **Privileges**: Must be run as Administrator (or with `SeDebugPrivilege`)
- **Compiler**:
  - Windows: Visual Studio 2022 / Build Tools
  - Linux: `mingw-w64` (for cross-compilation)

---

## 📦 Build Instructions

### 1. Save the code as `main.c/`

### 2. Build on **Windows** (Developer Command Prompt for VS)

```cmd
cl.exe /W4 /O2 /MT main.c /link /OUT:main.exe
```

### 3. Build on **Ubuntu/Debian** (Cross-Compile with MinGW)

```cmd
x86_64-w64-mingw32-gcc main.c -o main.exe \
    -static -ladvapi32 -luserenv -lkernel32 \
    -s -O2 -fomit-frame-pointer -fno-stack-protector

x86_64-w64-mingw32-gcc token_nt_direct.c -o token_nt_direct.exe \
    -static -ladvapi32 -luserenv -lkernel32 \
     -s -O2 -fomit-frame-pointer -fno-stack-protector

x86_64-w64-mingw32-gcc token_indirect_handle.c -o token_indirect_handle.exe \
    -static -ladvapi32 -luserenv -lkernel32 \
     -s -O2 -fomit-frame-pointer -fno-stack-protector

```

## 🚀 Usage

```cmd
main.exe <process_name>
token_nt_direct.exe <process_name>
token_indirect_handle.exe <process_name>
```

## Example
Find processes running as **NT Authority\System**

```cmd
tasklist /v /fi "USERNAME eq NT AUTHORITY\SYSTEM"
```
Then execute 
```cmd
main.exe services.exe
token_nt_direct.exe services.exe
token_indirect_handle.exe services.exe
```
### Expected Successful Output:
```cmd
[+] SeDebugPrivilege enabled.
[+] Found process: services.exe PID: 1234
[+] Target PID: 1234
[+] Got handle to PID 1234
[+] Opened process token.
[+] Token duplicated successfully.
[+] Process spawned successfully.
[+] PID: 5678 TID: 9012
[+] Done.
```

`A new cmd.exe window will appear running with the duplicated token (typically SYSTEM).`

## ⚠️ Important Notes & Warnings

- Educational / Research Use Only — This demonstrates Windows token mechanics.
- Requires administrative privileges to enable SeDebugPrivilege.
- May be detected by antivirus / EDR solutions (normal for token manipulation tools).
- Works best against SYSTEM services that are PPL-protected.
- The spawned cmd.exe inherits the full privilege set of the target process.


## 📂 Project Structure
```cmd
├── main.c          ← Main source code
├── main.exe        ← Compiled binary
├── token_nt_direct.c          ← Main source code
├── token_nt_direct.exe        ← Compiled binary
├── token_indirect_handle.c          ← Main source code
├── token_indirect_handle.exe        ← Compiled binary
└── README.md       ← This file
```

## 🔧 How It Works (High-Level)

1. EnableSeDebugPrivilege() — Grants debug rights to the current process
2. FindProcessPID() — Uses CreateToolhelp32Snapshot to locate target by name
3. get_process_handle() — Opens process with PROCESS_QUERY_LIMITED_INFORMATION
4. DuplicateProcessToken() — Opens token → DuplicateTokenEx (SecurityDelegation + MAXIMUM_ALLOWED)
5. EnableAllPrivileges() — Enables every privilege available on the new token
6. SpawnProcessWithToken() — CreateProcessWithTokenW to launch cmd.exe


## 📜 License
MIT License — feel free to use, modify, and distribute.

## ❤️ Disclaimer
Created as a clean, well-documented example of Windows token duplication.
Use responsibly and only on systems you own or have explicit permission to test.
The author is not responsible for any misuse.
