#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include "header/window_manager.h"
#include "header/config.h"

// Определяем глобальные переменные ТОЛЬКО здесь
BOOL serverRunning = FALSE;
SOCKET serverSocket = INVALID_SOCKET;
HANDLE hServerThread = NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ProcessMonitorServer";

    if (!RegisterClass(&wc)) return 1;

    HWND hwnd = CreateWindow("ProcessMonitorServer",
        "Process Monitor - Server",
        WS_OVERLAPPEDWINDOW | WS_SIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        850, 550,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}