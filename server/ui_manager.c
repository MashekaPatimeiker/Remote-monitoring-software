#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include "header/ui_manager.h"
#include "header/process_manager.h"
#include "header/config.h"
HWND hListBox, hStatus, hStartBtn, hRefreshBtn;

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

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    char statusMsg[256];
    sprintf(statusMsg, "Server: %s | Port: 8888 | Processes: %d",
            serverRunning ? "RUNNING" : "READY", count);
    UpdateStatus(statusMsg);

    char header[256];
    sprintf(header, "PROCESSES: %d (Sorted by Memory Usage)", count);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)header);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"");

    SortProcessesByMemory(processes, count);

    char buffer[1024];

    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"#   PROCESS                        PID      MEMORY(KB)  THREADS");
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"------------------------------------------------------------------");

    for (int i = 0; i < count; i++) {
        if (processes[i].memory > 0) {
            _snprintf(buffer, sizeof(buffer) - 1,
                     "%3d %-25s  %7lu  %10lu  %7lu",
                     i + 1, processes[i].name, processes[i].pid,
                     processes[i].memory, processes[i].threadCount);
        } else {
            _snprintf(buffer, sizeof(buffer) - 1,
                     "%3d %-25s  %7lu  %10s  %7lu",
                     i + 1, processes[i].name, processes[i].pid,
                     "N/A", processes[i].threadCount);
        }
        buffer[sizeof(buffer) - 1] = '\0';
        SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
    }

    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"");
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)"STATISTICS:");

    DWORD totalMemory = 0;
    DWORD totalThreads = 0;
    for (int i = 0; i < count; i++) {
        totalMemory += processes[i].memory;
        totalThreads += processes[i].threadCount;
    }

    _snprintf(buffer, sizeof(buffer) - 1,
             "Total Memory: %lu KB (%.1f MB)",
             totalMemory, totalMemory / 1024.0);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);

    _snprintf(buffer, sizeof(buffer) - 1,
             "Total Threads: %lu | Avg per process: %.1f",
             totalThreads, count > 0 ? (float)totalThreads / count : 0);
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buffer);
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