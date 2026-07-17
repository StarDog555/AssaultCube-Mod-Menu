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

void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <1 to Enable | 0 to Disable>\n";
    std::cout << "Example: " << programName << " 1  (Turns on Infinite Grenades)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string choice = argv[1];
    bool enableInfiniteGrenades = (choice == "1");

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

    // Target Instruction Offset (ac_client.exe + 0xC7E5D)
    uintptr_t targetOffset = 0xC7E5D;
    uintptr_t patchAddress = baseAddress + targetOffset;

    // The instruction to overwrite is 2 bytes long: FF 08 (dec [eax])
    const size_t patchSize = 2;
    BYTE originalBytes[patchSize] = { 0xFF, 0x08 };
    BYTE nopBytes[patchSize]      = { 0x90, 0x90 }; // Overwrite with 2 NOPs

    BYTE* bytesToWrite = enableInfiniteGrenades ? nopBytes : originalBytes;

    std::cout << "[+] Base address: 0x" << std::hex << baseAddress << "\n";
    std::cout << "[+] Target Instruction Address: 0x" << patchAddress << std::dec << "\n";

    // Modify memory protection flags to allow writing
    DWORD oldProtect;
    if (VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(patchAddress), patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        
        BOOL success = WriteProcessMemory(
            hProcess,                                  
            reinterpret_cast<LPVOID>(patchAddress),     
            bytesToWrite,                             
            patchSize,                      
            NULL                                       
        );

        if (success) {
            if (enableInfiniteGrenades) {
                std::cout << "[+] Successfully Patched! Infinite Grenades are now ENABLED.\n";
            } else {
                std::cout << "[+] Successfully Restored! Normal grenade behavior restored.\n";
            }
        } else {
            std::cout << "[-] Failed to write memory. Error code: " << GetLastError() << "\n";
        }

        // Restore original memory protection rules
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(patchAddress), patchSize, oldProtect, &oldProtect);
    } 
    else {
        std::cout << "[-] VirtualProtectEx failed. Error code: " << GetLastError() << "\n";
    }

    CloseHandle(hProcess);
    return 0;
}