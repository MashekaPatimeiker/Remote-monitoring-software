#include "header/process_manager.h"
#include <string.h>

int GetProcesses(ProcessInfo* processes, int max) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    int count = 0;
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (count >= max) break;

            // Безопасное копирование имени процесса (фикс warning)
            int copySize = sizeof(processes[count].name) - 1;
            if (strlen(pe.szExeFile) > copySize) {
                // Обрезаем слишком длинное имя
                strncpy(processes[count].name, pe.szExeFile, copySize - 3);
                strcat(processes[count].name, "...");
            } else {
                strcpy(processes[count].name, pe.szExeFile);
            }

            processes[count].pid = pe.th32ProcessID;
            processes[count].threadCount = pe.cntThreads;
            processes[count].memory = GetProcessMemoryUsage(pe.th32ProcessID);

            count++;
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return count;
}

DWORD GetProcessMemoryUsage(DWORD pid) {
    DWORD memory = 0;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                                 FALSE, pid);
    if (hProcess) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            memory = pmc.WorkingSetSize / 1024;
        }
        CloseHandle(hProcess);
    } else {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                memory = pmc.WorkingSetSize / 1024;
            }
            CloseHandle(hProcess);
        }
    }
    return memory;
}

DWORD GetTotalThreadCount() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    DWORD threadCount = 0;
    if (Thread32First(hSnapshot, &te)) {
        do {
            threadCount++;
        } while (Thread32Next(hSnapshot, &te));
    }

    CloseHandle(hSnapshot);
    return threadCount;
}

void SortProcessesByMemory(ProcessInfo* processes, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (processes[i].memory < processes[j].memory) {
                ProcessInfo temp = processes[i];
                processes[i] = processes[j];
                processes[j] = temp;
            }
        }
    }
}