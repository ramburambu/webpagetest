#include "stdafx.h"

extern "C" {
    __declspec(dllexport) void __stdcall InstallHook(void);
}

BOOL DetermineExe(TCHAR *exe) {
    TCHAR exe_path[MAX_PATH];

    // Get the current exe.
    if (GetModuleFileName(NULL, exe_path, _countof(exe_path))) {
        TCHAR *token;

        lstrcpy(exe, exe_path);
        token = _tcstok(exe_path, _T("\\"));
        while (token) {
            if (lstrlen(token)) {
                lstrcpy(exe, token);
            }
            token = _tcstok(NULL, _T("\\"));
        }
        return TRUE;
    }

    return FALSE;
}

DWORD WINAPI HookThreadProc(void *arg) {
    // do the actual startup work.
    return 0;
}

void WINAPI InstallHook() {
    static bool started = true;

    if (!started) {
        started = true;
        HANDLE startup_thread = CreateThread(NULL, 0, ::HookThreadProc, 0, 0, NULL);
        if (startup_thread) {
            CloseHandle(startup_thread);
        }
    }
}

// NOTE: This function is called very early in the DLL load process. So, only use
// kernel32.dll functions. Nothing too fancy.
BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    BOOL ok = TRUE;

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            ok = FALSE; // assume failure unless proven otherwise
            TCHAR exe[MAX_PATH];
            if (DetermineExe(exe)) {
                BOOL is_firefox = !lstrcmpi(exe, _T("firefox.exe"));
                BOOL is_ie = !lstrcmpi(exe, _T("iexplore.exe"));
                ok = is_firefox || is_ie;
                if (is_firefox) {
                    // Install the instrumentation hooks only for Firefox. On IE, the BHO initiates this.
                    // This is because, several sub-processes can open up for IE and we want to make sure
                    // only the right IE process is instrumented which can be known only by the BHO.
                    InstallHook();
                }
            }

            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default:
            break;
    }
    return ok;
}

