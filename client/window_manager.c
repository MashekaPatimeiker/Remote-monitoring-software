#include "header/window_manager.h"
#include "header/ui_manager.h"
#include "header/client.h"
#include "header/scanner.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateUIElements(hwnd);
            break;

        case WM_NOTIFY:
            HandleNotify(hwnd, wParam, lParam);
            break;

        case WM_COMMAND:
            HandleCommand(hwnd, wParam, lParam);
            break;

        case WM_TIMER:
            if (wParam == 1 && autoRefresh) {
                ConnectToServer();
            }
            break;

        case WM_SIZE:
            HandleSize(hwnd, wParam, lParam);
            break;

        case WM_USER + 2: // Scan completed - ИСПРАВЛЕНО
            UpdateConnectionStatus((const wchar_t*)lParam);
            scanThreadRunning = FALSE;
            hScanThread = NULL;
            break;

        case WM_USER + 3: // Update status
            UpdateConnectionStatus((const wchar_t*)lParam);
            break;

        case WM_USER + 4: // Add scan result
            AddScanResult((const wchar_t*)lParam);
            break;

        case WM_USER + 5: // Clear results
            ClearScanResults();
            break;

        case WM_CLOSE:
            // Останавливаем автообновление
            if (refreshTimer) {
                KillTimer(hwnd, refreshTimer);
                refreshTimer = 0;
            }

            // Останавливаем сканирование
            if (scanThreadRunning) {
                StopScanning();
            }

            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void HandleCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (LOWORD(wParam) == ID_BUTTON_CONN) {
        GetWindowTextW(hEditIP, serverIP, sizeof(serverIP)/sizeof(wchar_t));
        ConnectToServer();
    }
    else if (LOWORD(wParam) == ID_REFRESH) {
        ConnectToServer();
    }
    else if (LOWORD(wParam) == ID_AUTO) {
        HandleAutoRefresh(hwnd);
    }
    else if (LOWORD(wParam) == ID_SCAN_NETWORK) {
        if (!scanThreadRunning && !hScanThread) {  // Добавлена проверка
            // Очищаем результаты
            ClearScanResults();

            // Сбрасываем флаг перед созданием потока
            scanThreadRunning = FALSE;

            hScanThread = CreateThread(NULL, 0, ScanNetworkThread, hwnd, 0, NULL);
            if (hScanThread) {
                // Не закрываем handle здесь - закроем при завершении
            }
        }
    }
    else if (LOWORD(wParam) == ID_CONNECT_TO_DEV) {
        ConnectToSelectedDevice();
    }
}

void HandleNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
        int selectedTab = TabCtrl_GetCurSel(hTabControl);

        // Show/hide elements depending on tab
        if (selectedTab == 0) { // Manual Connection
            ShowWindow(hEditIP, SW_SHOW);
            ShowWindow(hConnectBtn, SW_SHOW);
            ShowWindow(hRefreshBtn, SW_SHOW);
            ShowWindow(hAutoCheck, SW_SHOW);
            ShowWindow(hListBox, SW_SHOW);

            ShowWindow(hScanBtn, SW_HIDE);
            ShowWindow(hConnectToDeviceBtn, SW_HIDE);
            ShowWindow(hScanListBox, SW_HIDE);
        } else { // Network Scan
            ShowWindow(hEditIP, SW_HIDE);
            ShowWindow(hConnectBtn, SW_HIDE);
            ShowWindow(hRefreshBtn, SW_HIDE);
            ShowWindow(hAutoCheck, SW_HIDE);
            ShowWindow(hListBox, SW_HIDE);

            ShowWindow(hScanBtn, SW_SHOW);
            ShowWindow(hConnectToDeviceBtn, SW_SHOW);
            ShowWindow(hScanListBox, SW_SHOW);
        }
    }
}

void HandleSize(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    LayoutUI(hwnd, width, height);
}