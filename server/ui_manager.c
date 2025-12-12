#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include "header/ui_manager.h"
#include "header/process_manager.h"
#include "header/config.h"
HWND hListBox, hStatus, hStartBtn, hRefreshBtn;

DWORD CalculateTotalMemory(ProcessInfo* processes, int count) {
    DWORD totalMemory = 0;
    for (int i = 0; i < count; i++) {
        totalMemory += processes[i].memory;
    }
    return totalMemory;
}
int GetProcesses(ProcessInfo* processes, int maxProcesses);
void DisplayThreadStatistics(DWORD totalThreads, int processCount) {
    char buffer[1024];
    float avgThreads = processCount > 0 ? (float)totalThreads / processCount : 0;

    _snprintf(buffer, sizeof(buffer) - 1,
             "Total Threads: %lu | Avg per process: %.1f",
             totalThreads, avgThreads);
    buffer[sizeof(buffer) - 1] = '\0';
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
}
void PrepareProcessListDisplay() {
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
}

void UpdateServerStatus(int processCount) {
    char statusMsg[256];
    sprintf(statusMsg, "Server: %s | Port: 8888 | Processes: %d",
            serverRunning ? "RUNNING" : "READY", processCount);
    UpdateStatus(statusMsg);
}

void DisplayListHeaders(int processCount) {
    char header[256];
    sprintf(header, "PROCESSES: %d (Sorted by Memory Usage)", processCount);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)header);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"");

    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"#   PROCESS                        PID      MEMORY(KB)  THREADS");
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"------------------------------------------------------------------");
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
void FormatProcessLine(ProcessInfo process, int index, char* buffer, size_t bufferSize) {
    if (process.memory > 0) {
        _snprintf(buffer, bufferSize - 1,
                 "%3d %-25s  %7lu  %10lu  %7lu",
                 index, process.name, process.pid,
                 process.memory, process.threadCount);
    } else {
        _snprintf(buffer, bufferSize - 1,
                 "%3d %-25s  %7lu  %10s  %7lu",
                 index, process.name, process.pid,
                 "N/A", process.threadCount);
    }
    buffer[bufferSize - 1] = '\0';
}

void DisplayProcessesList(ProcessInfo* processes, int count) {
    char buffer[1024];

    for (int i = 0; i < count; i++) {
        FormatProcessLine(processes[i], i + 1, buffer, sizeof(buffer));
        SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
    }
}


DWORD CalculateTotalThreads(ProcessInfo* processes, int count) {
    DWORD totalThreads = 0;
    for (int i = 0; i < count; i++) {
        totalThreads += processes[i].threadCount;
    }
    return totalThreads;
}

void DisplayMemoryStatistics(DWORD totalMemory, int processCount) {
    char buffer[1024];
    _snprintf(buffer, sizeof(buffer) - 1,
             "Total Memory: %lu KB (%.1f MB)",
             totalMemory, totalMemory / 1024.0);
    buffer[sizeof(buffer) - 1] = '\0';
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
}


void DisplayProcessStatistics(ProcessInfo* processes, int count) {
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"");
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"STATISTICS:");

    DWORD totalMemory = CalculateTotalMemory(processes, count);
    DWORD totalThreads = CalculateTotalThreads(processes, count);

    DisplayMemoryStatistics(totalMemory, count);

    DisplayThreadStatistics(totalThreads, count);
}



void CreateUIElements(HWND hwnd) {
    hStartBtn = CreateWindow("BUTTON", "Start Server",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        30, 20, 150, 40, hwnd, (HMENU)ID_BUTTON_START, NULL, NULL);

    hRefreshBtn = CreateWindow("BUTTON", "Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        190, 20, 120, 40, hwnd, (HMENU)ID_REFRESH, NULL, NULL);

    hListBox = CreateWindow("LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        30, 70, 740, 350, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);

    hStatus = CreateWindow("STATIC", "Server: READY | Port: 8888 | Processes: 0",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_SUNKEN,
        10, 430, 780, 30, hwnd, (HMENU)ID_STATUS, NULL, NULL);

    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
    SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

    HFONT hStatusFont = CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
    SendMessage(hStatus, WM_SETFONT, (WPARAM)hStatusFont, TRUE);

    UpdateProcessList();
}
void UpdateProcessList() {
    ProcessInfo processes[MAX_PROCESSES];
    int count = GetProcesses(processes, MAX_PROCESSES);

    PrepareProcessListDisplay();

    UpdateServerStatus(count);

    DisplayListHeaders(count);

    SortProcessesByMemory(processes, count);

    DisplayProcessesList(processes, count);

    DisplayProcessStatistics(processes, count);
}

void UpdateStatus(const char* status) {
    SetWindowTextA(hStatus, status);
}

void LayoutUI(HWND hwnd, int width, int height) {
    MoveWindow(hStartBtn, 30, 20, 150, 40, TRUE);
    MoveWindow(hRefreshBtn, 190, 20, 120, 40, TRUE);
    MoveWindow(hListBox, 30, 70, width - 60, height - 110, TRUE);
    MoveWindow(hStatus, 10, height - 35, width - 20, 30, TRUE);
}