#include "header/client.h"
#include "header/ui_manager.h"
#include "header/network_utils.h"

// Function to extract statistics from received data
void ExtractAndDisplayStats(const wchar_t* buffer) {
    const wchar_t* possibleHeaders[] = {
        L"=== SYSTEM INFORMATION ===",
        L"=== System Information ===",
        L"=== Statistics ===",
        L"=== STATISTICS ==="
    };

    wchar_t* statStart = NULL;
    for (int i = 0; i < 4; i++) {
        statStart = wcsstr(buffer, possibleHeaders[i]);
        if (statStart) break;
    }

    if (statStart) {
        wchar_t stats[512];
        wchar_t* lineStart = statStart;
        wchar_t* nextLine = wcschr(lineStart, L'\n');

        if (nextLine) {
            // Copy header and next line
            wcsncpy(stats, lineStart, 511);
            stats[511] = L'\0';

            // Truncate at first newline if it exists in copied string
            wchar_t* newlineInStats = wcschr(stats, L'\n');
            if (newlineInStats) {
                *newlineInStats = L'\0';
            }

            SetWindowTextW(hStats, stats);
        }
    }
}

// Альтернативная функция для разбиения строк без wcstok
void AddLinesToListBox(const wchar_t* buffer) {
    wchar_t lineBuffer[2048];
    int linePos = 0;
    int bufferPos = 0;

    while (buffer[bufferPos] != L'\0' && linePos < 2047) {
        if (buffer[bufferPos] == L'\n' || buffer[bufferPos] == L'\r') {
            if (linePos > 0) {
                lineBuffer[linePos] = L'\0';
                SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
                linePos = 0;
            }

            // Пропускаем все символы новой строки подряд
            while (buffer[bufferPos] == L'\n' || buffer[bufferPos] == L'\r') {
                bufferPos++;
            }
        } else {
            lineBuffer[linePos++] = buffer[bufferPos++];
        }
    }

    // Добавляем последнюю строку, если она есть
    if (linePos > 0) {
        lineBuffer[linePos] = L'\0';
        SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)lineBuffer);
    }
}

void ConnectToServer() {
    // Convert IP to ANSI for network functions
    char ansiIP[MAX_IP_LENGTH];
    UnicodeToAnsi(serverIP, ansiIP, sizeof(ansiIP));

    // Clear list and update status
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    SetWindowTextW(hStats, L"");
    UpdateConnectionStatus(L"Connecting...");

    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        UpdateConnectionStatus(L"Winsock initialization error");
        return;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        UpdateConnectionStatus(L"Socket creation error");
        return;
    }

    // Setup server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, ansiIP, &serverAddr.sin_addr);

    // Set timeout
    int timeout = 3000; // 3 seconds
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(clientSocket);
        WSACleanup();

        wchar_t errorMsg[256];
        int error = WSAGetLastError();

        switch (error) {
            case WSAECONNREFUSED:
                swprintf(errorMsg, 256, L"Connection refused. Server not running on %s:%d", serverIP, PORT);
                break;
            case WSAETIMEDOUT:
                swprintf(errorMsg, 256, L"Connection timeout. Check IP: %s", serverIP);
                break;
            case WSAEHOSTUNREACH:
                swprintf(errorMsg, 256, L"Host unreachable: %s", serverIP);
                break;
            default:
                swprintf(errorMsg, 256, L"Connection error %d to %s:%d", error, serverIP, PORT);
        }

        UpdateConnectionStatus(errorMsg);
        return;
    }

    UpdateConnectionStatus(L"Connected, receiving data...");

    // Receive data
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';

        // Convert received data to Unicode for display
        wchar_t wbuffer[BUFFER_SIZE];
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, BUFFER_SIZE);

        // Try other codecs if UTF-8 fails
        if (wcslen(wbuffer) == 0) {
            MultiByteToWideChar(CP_ACP, 0, buffer, -1, wbuffer, BUFFER_SIZE);
        }

        // Используем альтернативную функцию вместо wcstok
        AddLinesToListBox(wbuffer);

        // Extract and display statistics
        ExtractAndDisplayStats(wbuffer);

        // Update status
        wchar_t successMsg[256];
        swprintf(successMsg, 256, L"Успех! Получено %d байт от %s", bytesReceived, serverIP);
        UpdateConnectionStatus(successMsg);

        // Save to log file
        FILE* file = _wfopen(L"monitor_log.txt", L"a");
        if (file) {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
            fprintf(file, "\n=== %s ===\n", timeStr);
            fprintf(file, "Server: %s:%d\n", ansiIP, PORT);
            fprintf(file, "%s\n", buffer);
            fclose(file);
        }
    } else if (bytesReceived == 0) {
        UpdateConnectionStatus(L"Сервер закрыл соединение");
    } else {
        UpdateConnectionStatus(L"Данные от сервера не получены");
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();
}

void ConnectToSelectedDevice() {
    // If server found, connect to it
    if (deviceCount > 0) {
        for (int i = 0; i < deviceCount; i++) {
            if (devices[i].hasServer) {
                // Set found IP in edit field
                SetWindowTextW(hEditIP, devices[i].ip);
                GetWindowTextW(hEditIP, serverIP, sizeof(serverIP)/sizeof(wchar_t));

                // Switch to first tab
                TabCtrl_SetCurSel(hTabControl, 0);

                // Show first tab elements
                ShowWindow(hEditIP, SW_SHOW);
                ShowWindow(hConnectBtn, SW_SHOW);
                ShowWindow(hRefreshBtn, SW_SHOW);
                ShowWindow(hAutoCheck, SW_SHOW);
                ShowWindow(hListBox, SW_SHOW);
                ShowWindow(hScanBtn, SW_HIDE);
                ShowWindow(hConnectToDeviceBtn, SW_HIDE);
                ShowWindow(hScanListBox, SW_HIDE);  // ✅ Используем hScanListBox

                // Connect
                ConnectToServer();
                break;
            }
        }
    } else {
        UpdateConnectionStatus(L"Server not found. Run network scan first.");
    }
}

void HandleAutoRefresh(HWND hwnd) {
    autoRefresh = !autoRefresh;
    if (autoRefresh) {
        refreshTimer = SetTimer(hwnd, 1, 5000, NULL); // 5 seconds
        ConnectToServer();
    } else {
        if (refreshTimer) {
            KillTimer(hwnd, refreshTimer);
            refreshTimer = 0;
        }
    }
}