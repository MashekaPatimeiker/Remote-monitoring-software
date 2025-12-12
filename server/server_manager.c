#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "header/server_manager.h"
#include "header/process_manager.h"
#include "header/ui_manager.h"
#include "header/config.h"

extern BOOL serverRunning;
extern SOCKET serverSocket;
extern HANDLE hServerThread;
void FormatClientResponse(char* buffer, int bufferSize, ProcessInfo* processes, int count,
                         const char* hostname, const char* clientIP) {
    int pos = 0;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);

    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "--- REMOTE PROCESS MONITOR ---\n");
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Time: %s\n", timeStr);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Server: %s\n", hostname);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Client IP: %s\n", clientIP);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Total Processes: %d\n\n", count);

    DWORD totalMem = 0;
    DWORD systemProcesses = 0;
    DWORD userProcesses = 0;
    DWORD totalThreads = 0;

    for (int i = 0; i < count; i++) {
        totalMem += processes[i].memory;
        totalThreads += processes[i].threadCount;
        if (processes[i].pid <= 4) {
            systemProcesses++;
        } else {
            userProcesses++;
        }
    }

    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "--- MEMORY STATISTICS ---\n");
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Total Memory Used: %lu KB (%.2f MB)\n",
                  totalMem, totalMem / 1024.0);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "System Processes: %lu\n", systemProcesses);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "User Processes: %lu\n", userProcesses);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Total Threads: %lu\n", totalThreads);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "Average Memory per Process: %lu KB\n",
                  count > 0 ? totalMem / count : 0);
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "\n");

    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "--- ALL PROCESSES (Sorted by Memory Usage) ---\n");
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "No.  Process Name                     PID        Memory (KB)   Threads\n");
    pos += _snprintf(buffer + pos, bufferSize - pos - 1, "------------------------------------------------------------------------\n");

    for (int i = 0; i < count && pos < bufferSize - 512; i++) {
        if (processes[i].memory > 0) {
            pos += _snprintf(buffer + pos, bufferSize - pos - 1,
                          "%3d. %-30s  %8lu  %12lu  %8lu\n",
                          i + 1, processes[i].name, processes[i].pid,
                          processes[i].memory, processes[i].threadCount);
        } else {
            pos += _snprintf(buffer + pos, bufferSize - pos - 1,
                          "%3d. %-30s  %8lu  %12s  %8lu\n",
                          i + 1, processes[i].name, processes[i].pid,
                          "N/A", processes[i].threadCount);
        }
    }

    if (pos >= bufferSize) {
        pos = bufferSize - 1;
    }
    buffer[pos] = '\0';
}

DWORD WINAPI ServerThread(LPVOID lpParam) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt));

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        char error[256];
        _snprintf(error, sizeof(error) - 1, "Bind failed: %d", WSAGetLastError());
        error[sizeof(error) - 1] = '\0';
        UpdateStatus(error);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 5);

    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    char info[512];
    _snprintf(info, sizeof(info) - 1, "Server: RUNNING on %s:%d", hostname, PORT);
    info[sizeof(info) - 1] = '\0';
    UpdateStatus(info);

    serverRunning = TRUE;
    SetWindowText(hStartBtn, "Stop Server");

    while (serverRunning) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        struct timeval timeout = {1, 0};

        if (select(0, &readSet, NULL, NULL, &timeout) > 0) {
            struct sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);

            if (clientSocket != INVALID_SOCKET) {
                ProcessInfo processes[MAX_PROCESSES];
                int count = GetProcesses(processes, MAX_PROCESSES);
                SortProcessesByMemory(processes, count);

                char buffer[BUFFER_SIZE];
                char clientIP[16];
                strcpy(clientIP, inet_ntoa(clientAddr.sin_addr));

                FormatClientResponse(buffer, BUFFER_SIZE, processes, count, hostname, clientIP);

                int bytesSent = send(clientSocket, buffer, strlen(buffer), 0);

                char logMsg[256];
                _snprintf(logMsg, sizeof(logMsg) - 1,
                         "Sent %d bytes to %s (showing all %d processes)",
                         bytesSent, clientIP, count);
                logMsg[sizeof(logMsg) - 1] = '\0';
                UpdateStatus(logMsg);

                closesocket(clientSocket);
                UpdateProcessList();
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    serverRunning = FALSE;
    SetWindowText(hStartBtn, "Start Server");
    UpdateStatus("Server: STOPPED");

    return 0;
}

void StopServer() {
    serverRunning = FALSE;
    if (hServerThread) {
        WaitForSingleObject(hServerThread, 1000);
        CloseHandle(hServerThread);
        hServerThread = NULL;
    }
}

BOOL IsServerRunning() {
    return serverRunning;
}