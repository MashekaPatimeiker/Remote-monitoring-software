#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "config.h"

int GetProcesses(ProcessInfo* processes, int max);
DWORD GetProcessMemoryUsage(DWORD pid);
DWORD GetTotalThreadCount();
void SortProcessesByMemory(ProcessInfo* processes, int count);

#endif