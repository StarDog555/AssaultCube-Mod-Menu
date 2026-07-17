#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <tchar.h>

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
        std::cout << "Usage: InfAmo.exe <ammo_amount>\n";
        return 1;
    }

    int newAmmoValue = std::stoi(argv[1]);
    const TCHAR* processName = _T("ac_client.exe");
    
    DWORD processID = GetProcId(processName);
    if (processID == 0) {
        std::wcout << L"[-] Error: Game process not found. Make sure the game is running.\n";
        return 1;
    }

    uintptr_t baseAddress = GetModuleBaseAddress(processID, processName);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (hProcess == NULL) {
        std::cout << "[-] Error: Run as Administrator!\n";
        return 1;
    }

    // --- ASSAULTCUBE v1.3.0.2 OFFSETS ---
    uintptr_t localPlayerPtrOffset = 0x17E0A8; // Corrected v1.3.0.2 Local Player Base Pointer
    uintptr_t ammoOffset = 0x140;              // Corrected v1.3.0.2 Rifle/Active ammo offset

    uintptr_t localPlayerAddress = baseAddress + localPlayerPtrOffset;

    // Read the 32-bit player structure base pointer
    DWORD playerBaseAddress = 0; 
    BOOL readSuccess = ReadProcessMemory(
        hProcess, 
        reinterpret_cast<LPCVOID>(localPlayerAddress), 
        &playerBaseAddress, 
        sizeof(DWORD), 
        NULL
    );

    if (!readSuccess || playerBaseAddress == 0) {
        std::cout << "[-] Error: Failed to read player pointer. Are you fully spawned in a match?\n";
        CloseHandle(hProcess);
        return 1;
    }

    DWORD ammoAddress = playerBaseAddress + ammoOffset;

    std::cout << "[+] Player Base Address: 0x" << std::hex << playerBaseAddress << "\n";
    std::cout << "[+] Target Ammo Address: 0x" << ammoAddress << std::dec << "\n";

    // Gain temporary permission and write the value
    DWORD oldProtect;
    if (VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(ammoAddress), sizeof(newAmmoValue), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        
        BOOL success = WriteProcessMemory(
            hProcess,                                  
            reinterpret_cast<LPVOID>(ammoAddress),     
            &newAmmoValue,                             
            sizeof(newAmmoValue),                      
            NULL                                       
        );

        if (success) {
            std::cout << "[+] Successfully updated ammo to " << newAmmoValue << "!\n";
        } else {
            std::cout << "[-] Failed to write memory. Error code: " << GetLastError() << "\n";
        }

        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(ammoAddress), sizeof(newAmmoValue), oldProtect, &oldProtect);
    } 
    else {
        std::cout << "[-] VirtualProtectEx failed. Error code: " << GetLastError() << "\n";
    }

    CloseHandle(hProcess);
    return 0;
}