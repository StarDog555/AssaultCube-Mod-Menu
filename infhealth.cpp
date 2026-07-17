#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: GodMode.exe <health_value>\n";
        std::cout << "Example: GodMode.exe 9999\n";
        return 1;
    }

    int targetHealthValue = std::stoi(argv[1]);
    const TCHAR* processName = _T("ac_client.exe");
    
    DWORD processID = GetProcId(processName);
    if (processID == 0) {
        std::wcout << L"[-] Error: Game process not found.\n";
        return 1;
    }

    uintptr_t baseAddress = GetModuleBaseAddress(processID, processName);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cout << "[-] Error: Run as Administrator!\n";
        return 1;
    }

    // --- ASSAULTCUBE v1.3.0.2 OFFSETS ---
    uintptr_t localPlayerPtrOffset = 0x17E0A8; // Local Player Pointer base
    uintptr_t healthOffset = 0xEC;             // Health offset inside the player structure

    uintptr_t localPlayerAddress = baseAddress + localPlayerPtrOffset;

    std::cout << "[+] God Mode Activated! Hold 'END' key to close this tool.\n";
    std::cout << "[+] Freezing player health to: " << targetHealthValue << "\n\n";

    // Loop continuously in the background to keep freezing your health
    while (!GetAsyncKeyState(VK_END)) {
        DWORD playerBaseAddress = 0; 
        BOOL readSuccess = ReadProcessMemory(
            hProcess, 
            reinterpret_cast<LPCVOID>(localPlayerAddress), 
            &playerBaseAddress, 
            sizeof(DWORD), 
            NULL
        );

        if (readSuccess && playerBaseAddress != 0) {
            DWORD healthAddress = playerBaseAddress + healthOffset;

            // Write the custom health value
            WriteProcessMemory(
                hProcess,                                  
                reinterpret_cast<LPVOID>(healthAddress),     
                &targetHealthValue,                             
                sizeof(targetHealthValue),                      
                NULL                                       
            );
        }
        
        // Pause for 10 milliseconds to keep CPU usage at 0%
        Sleep(10);
    }

    std::cout << "[+] Shutting down tool...\n";
    CloseHandle(hProcess);
    return 0;
}