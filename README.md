# Tokenizer
A clean, single-file utility to duplicate a target process's token
# Windows Token Duplication Tool

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

### 1. Save the code as `main.c`

### 2. Build on **Windows** (Developer Command Prompt for VS)

```cmd
cl.exe /W4 /O2 /MT main.c /link /OUT:main.exe
```

### 3. Build on **Ubuntu/Debian (Cross-Compile with MinGW

```cmd
x86_64-w64-mingw32-gcc main.c -o main.exe \
    -static -ladvapi32 -luserenv -lkernel32 \
    -s -O2 -fomit-frame-pointer -fno-stack-protector
```

## 🚀 Usage

```cmd
main.exe <process_name>
```

## Example

```cmd
main.exe services.exe
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

