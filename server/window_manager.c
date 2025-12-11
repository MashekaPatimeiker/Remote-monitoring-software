#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include "header/window_manager.h"
#include "header/ui_manager.h"
#include "header/server_manager.h"

extern BOOL serverRunning;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateUIElements(hwnd);
            break;

        case WM_COMMAND:
            HandleCommand(hwnd, wParam, lParam);
            break;

        case WM_SIZE:
            HandleSize(hwnd, wParam, lParam);
            break;

        case WM_CLOSE:
            StopServer();
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void HandleCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (LOWORD(wParam) == ID_BUTTON_START) {
        if (!serverRunning) {
            hServerThread = CreateThread(NULL, 0, ServerThread, hwnd, 0, NULL);
        } else {
            StopServer();
        }
    }
    else if (LOWORD(wParam) == ID_REFRESH) {
        UpdateProcessList();
    }
}

void HandleSize(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    LayoutUI(hwnd, width, height);
}