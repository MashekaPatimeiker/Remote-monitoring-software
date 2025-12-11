#include "header/window_manager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ClientWindowClass";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc)) return 1;

    HWND hwnd = CreateWindowW(L"ClientWindowClass", L"Process Monitor - Client",
                            WS_OVERLAPPEDWINDOW | WS_SIZEBOX | WS_MAXIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            850, 600,
                            NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}