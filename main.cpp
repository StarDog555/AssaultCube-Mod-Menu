#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <tchar.h>
#include <process.h>

// Link libraries
#pragma comment(lib, "user32.lib")

// Control IDs
#define ID_CHKBX_HEALTH    101
#define ID_CHKBX_AMMO      102
#define ID_CHKBX_GRENADES  103
#define ID_TIMER_STATUS    201

// Global variables for game hacking (Marked volatile/synchronized where necessary)
volatile HANDLE hProcess = NULL;
volatile uintptr_t baseAddress = 0;
bool g_InfHealth = false;
bool g_InfAmmo = false;
bool g_InfGrenades = false;

// Game Offsets (AssaultCube v1.3.0.2 - 32-bit offsets)
const uintptr_t localPlayerPtrOffset = 0x17E0A8;
const uintptr_t healthOffset = 0xEC;
const uintptr_t ammoOffset = 0x140;
const uintptr_t grenadeOffset = 0x144; 

// UI elements
HWND hStatusLabel;
HWND hChkHealth, hChkAmmo, hChkGrenades;

// --- MEMORY UTILITIES ---

DWORD GetProcId(const TCHAR* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry)) {
            do {
                if (_tcsicmp(procEntry.szExeFile, procName) == 0) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const TCHAR* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (_tcsicmp(modEntry.szModule, modName) == 0) {
                    modBaseAddr = reinterpret_cast<uintptr_t>(modEntry.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

bool TryConnectToGame() {
    if (hProcess != NULL) {
        // Double check if the handle is still valid/active
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            return true; 
        }
        // Handle is dead, clean it up
        CloseHandle(hProcess);
        hProcess = NULL;
        baseAddress = 0;
    }

    DWORD processID = GetProcId(_T("ac_client.exe"));
    if (processID == 0) return false;

    baseAddress = GetModuleBaseAddress(processID, _T("ac_client.exe"));
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    
    return (hProcess != NULL);
}

// Background thread for continuous memory adjustments
unsigned __stdcall HackLoopThread(void* pArguments) {
    while (true) {
        // Cache variables locally to prevent multi-thread race hazards
        HANDLE localHProcess = hProcess;
        uintptr_t localBaseAddress = baseAddress;

        if (localHProcess != NULL && localBaseAddress != 0) {
            // Check if process died
            DWORD exitCode;
            if (GetExitCodeProcess(localHProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                hProcess = NULL;
                baseAddress = 0;
                Sleep(100);
                continue;
            }

            // --- Continuous Write Hacks ---
            DWORD playerBaseAddress = 0;
            uintptr_t localPlayerAddress = localBaseAddress + localPlayerPtrOffset;

            BOOL readSuccess = ReadProcessMemory(
                localHProcess,
                reinterpret_cast<LPCVOID>(localPlayerAddress),
                &playerBaseAddress,
                sizeof(DWORD),
                NULL
            );

            if (readSuccess && playerBaseAddress != 0) {
                // Freeze Health
                if (g_InfHealth) {
                    DWORD healthAddress = playerBaseAddress + healthOffset;
                    int newHealth = 9999;
                    WriteProcessMemory(localHProcess, reinterpret_cast<LPVOID>(healthAddress), &newHealth, sizeof(newHealth), NULL);
                }

                // Freeze Ammo
                if (g_InfAmmo) {
                    DWORD ammoAddress = playerBaseAddress + ammoOffset;
                    int newAmmo = 9999;
                    WriteProcessMemory(localHProcess, reinterpret_cast<LPVOID>(ammoAddress), &newAmmo, sizeof(newAmmo), NULL);
                }

                // Freeze Grenades
                if (g_InfGrenades) {
                    DWORD grenadeAddr = playerBaseAddress + grenadeOffset;
                    int grenadeVal = 1;
                    WriteProcessMemory(localHProcess, reinterpret_cast<LPVOID>(grenadeAddr), &grenadeVal, sizeof(grenadeVal), NULL);
                }
            }
        }
        Sleep(10); // Standard thread sleep to maintain low CPU footprint
    }
    return 0;
}

void SetHealth(int value)
{
    if (hProcess == NULL) return;

    DWORD playerBase = 0;
    if (ReadProcessMemory(hProcess,
        (LPCVOID)(baseAddress + localPlayerPtrOffset),
        &playerBase,
        sizeof(DWORD),
        NULL) && playerBase)
    {
        DWORD healthAddr = playerBase + healthOffset;
        WriteProcessMemory(hProcess, (LPVOID)healthAddr, &value, sizeof(value), NULL);
    }
}
void SetAmmo(int value)
{
    if (hProcess == NULL) return;

    DWORD playerBase = 0;
    if (ReadProcessMemory(hProcess,
        (LPCVOID)(baseAddress + localPlayerPtrOffset),
        &playerBase,
        sizeof(DWORD),
        NULL) && playerBase)
    {
        DWORD ammoAddr = playerBase + ammoOffset;
        WriteProcessMemory(hProcess, (LPVOID)ammoAddr, &value, sizeof(value), NULL);
    }
}

// --- WIN32 WINDOW PROCEDURE ---

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        hStatusLabel = CreateWindow(_T("STATIC"), _T("Status: Scanning for game..."),
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 15, 260, 20, hwnd, NULL, NULL, NULL);

        hChkHealth = CreateWindow(_T("BUTTON"), _T("Infinite Health (9999)"),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, 45, 260, 30, hwnd, (HMENU)ID_CHKBX_HEALTH, NULL, NULL);

        hChkAmmo = CreateWindow(_T("BUTTON"), _T("Infinite Ammo (9999)"),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, 80, 260, 30, hwnd, (HMENU)ID_CHKBX_AMMO, NULL, NULL);

        hChkGrenades = CreateWindow(_T("BUTTON"), _T("Infinite Grenades (Freeze to 1)"),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, 115, 260, 30, hwnd, (HMENU)ID_CHKBX_GRENADES, NULL, NULL);

        SetTimer(hwnd, ID_TIMER_STATUS, 1000, NULL);
        break;
    }

    case WM_TIMER: {
        if (wp == ID_TIMER_STATUS) {
            if (TryConnectToGame()) {
                SendMessage(hStatusLabel, WM_SETTEXT, 0, (LPARAM)_T("Status: Game Connected!"));
            } else {
                SendMessage(hStatusLabel, WM_SETTEXT, 0, (LPARAM)_T("Status: Game Closed. Waiting..."));
            }
        }
        break;
    }

    case WM_COMMAND: {
        if (HIWORD(wp) == BN_CLICKED) {
            int controlID = LOWORD(wp);
            bool isChecked = (SendMessage((HWND)lp, BM_GETCHECK, 0, 0) == BST_CHECKED);

            switch (controlID) {
            case ID_CHKBX_HEALTH:
                g_InfHealth = isChecked;
                if (!isChecked) SetHealth(100);
                break;
            
            case ID_CHKBX_AMMO:
                g_InfAmmo = isChecked;
                if (!isChecked) SetAmmo(20);
                break;

            case ID_CHKBX_GRENADES:
                g_InfGrenades = isChecked;
                break;
            }
        }
        break;
    }

    case WM_DESTROY: {
        if (hProcess != NULL) {
            CloseHandle(hProcess);
        }
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// --- MAIN ENTRY POINT ---

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    const TCHAR CLASS_NAME[] = _T("ModMenuWindowClass");

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        CLASS_NAME,
        _T("AssaultCube ModMenu v1.3.0.2"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 310, 195, // Height reduced to fit 3 options perfectly
        NULL, NULL, hInst, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Explicit cast for _beginthreadex prevents strict compilation errors
    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)HackLoopThread, NULL, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}