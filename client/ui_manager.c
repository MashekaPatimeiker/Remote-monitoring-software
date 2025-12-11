#include "header/ui_manager.h"
#include "header/client.h"

HWND hEditIP, hConnectBtn, hListBox, hStatus, hStats, hRefreshBtn, hAutoCheck;
HWND hScanBtn, hDeviceList, hConnectToDeviceBtn, hTabControl, hScanListBox;
wchar_t serverIP[MAX_IP_LENGTH] = L"192.168.0.100";
BOOL autoRefresh = FALSE;
UINT_PTR refreshTimer = 0;

void CreateUIElements(HWND hwnd) {
    hTabControl = CreateWindowW(WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        10, 10, 780, 450, hwnd, (HMENU)ID_TAB_CONTROL, NULL, NULL);

    TCITEMW tie;
    tie.mask = TCIF_TEXT;

    tie.pszText = L"Manual Connection";
    TabCtrl_InsertItem(hTabControl, 0, &tie);

    tie.pszText = L"Network Scan";
    TabCtrl_InsertItem(hTabControl, 1, &tie);

    CreateWindowW(L"STATIC", L"Server IP:",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        30, 50, 80, 30, hwnd, NULL, NULL, NULL);

    hEditIP = CreateWindowW(L"EDIT", serverIP,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        120, 50, 150, 30, hwnd, (HMENU)ID_EDIT_IP, NULL, NULL);

    hConnectBtn = CreateWindowW(L"BUTTON", L"Connect",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        280, 50, 120, 30, hwnd, (HMENU)ID_BUTTON_CONN, NULL, NULL);

    hRefreshBtn = CreateWindowW(L"BUTTON", L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        410, 50, 100, 30, hwnd, (HMENU)ID_REFRESH, NULL, NULL);

    hAutoCheck = CreateWindowW(L"BUTTON", L"Auto-refresh (5 sec)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        520, 50, 180, 30, hwnd, (HMENU)ID_AUTO, NULL, NULL);

    hListBox = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS,
        30, 100, 740, 300, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);

    hScanBtn = CreateWindowW(L"BUTTON", L"Scan Network",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        30, 50, 150, 35, hwnd, (HMENU)ID_SCAN_NETWORK, NULL, NULL);

    hConnectToDeviceBtn = CreateWindowW(L"BUTTON", L"Connect to Server",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        190, 50, 180, 35, hwnd, (HMENU)ID_CONNECT_TO_DEV, NULL, NULL);

    hScanListBox = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS,
        30, 100, 740, 300, hwnd, (HMENU)ID_SCAN_LISTBOX, NULL, NULL);

    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"Scan Results:");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"----------------------------------------");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"Click 'Scan Network' to start search");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"");

    // Status bar
    hStatus = CreateWindowW(L"STATIC", L"Ready to connect | Server: not selected",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10, 470, 780, 30, hwnd, (HMENU)ID_STATUS, NULL, NULL);

    hStats = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10, 505, 780, 40, hwnd, (HMENU)ID_STATS, NULL, NULL);

    // Hide tab 2 initially
    ShowWindow(hScanBtn, SW_HIDE);
    ShowWindow(hConnectToDeviceBtn, SW_HIDE);
    ShowWindow(hScanListBox, SW_HIDE);

    // Fonts with Unicode support
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Consolas");
    SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hScanListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Font for status
    HFONT hStatusFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    SendMessage(hStatus, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
    SendMessage(hStats, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
}

void UpdateConnectionStatus(const wchar_t* status) {
    SetWindowTextW(hStatus, status);
}

void AddScanResult(const wchar_t* result) {
    if (result == NULL) return;

    wchar_t cleanResult[1024];
    wcscpy(cleanResult, result);

    int len = wcslen(cleanResult);
    while (len > 0 && (cleanResult[len-1] == L'\n' || cleanResult[len-1] == L'\r')) {
        cleanResult[len-1] = L'\0';
        len--;
    }

    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)cleanResult);

    int itemCount = SendMessage(hScanListBox, LB_GETCOUNT, 0, 0);
    if (itemCount > 0) {
        SendMessage(hScanListBox, LB_SETTOPINDEX, itemCount - 1, 0);
    }

    free((void*)result);
}

void ClearScanResults() {
    SendMessage(hScanListBox, LB_RESETCONTENT, 0, 0);

    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"Scan Results:");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"----------------------------------------");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"Click 'Scan Network' to start search");
    SendMessageW(hScanListBox, LB_ADDSTRING, 0, (LPARAM)L"");

    SendMessage(hScanListBox, LB_SETTOPINDEX, 0, 0);
}

void LayoutUI(HWND hwnd, int width, int height) {
    MoveWindow(hTabControl, 10, 10, width - 20, height - 120, TRUE);

    MoveWindow(hListBox, 30, 100, width - 60, height - 230, TRUE);
    MoveWindow(hScanListBox, 30, 100, width - 60, height - 230, TRUE);

    MoveWindow(hStatus, 10, height - 100, width - 20, 30, TRUE);
    MoveWindow(hStats, 10, height - 65, width - 20, 40, TRUE);
}