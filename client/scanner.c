#include "header/scanner.h"
#include "header/network_utils.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define MAX_DEVICES 1000
#define MAX_THREADS 10
#define BATCH_SIZE 25

NetworkDevice devices[MAX_DEVICES];
int deviceCount = 0;
HANDLE hScanThread = NULL;
BOOL scanThreadRunning = FALSE;

CRITICAL_SECTION csDeviceList;
CRITICAL_SECTION csConsole;

typedef struct {
    char ip[16];
    char subnet[16];
} NetworkInterfaceInfo;

typedef struct {
    unsigned long* ipList;
    int count;
    HWND hwnd;
    int threadId;
} ScanTask;

BOOL QuickPortCheck(const char* ip, int port, int timeout_ms) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return FALSE;

    DWORD timeout = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;

    int result = select(0, NULL, &fdset, NULL, &tv);

    closesocket(sock);

    return (result == 1);
}

BOOL QuickPing(const char* ip, int timeout_ms) {
    return QuickPortCheck(ip, 445, timeout_ms);
}

int GetNetworkInterfaces(NetworkInterfaceInfo* interfaces, int maxInterfaces) {
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;
    int interfaceCount = 0;
    DWORD dwRetVal = 0;

    dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) return 0;

        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        if (dwRetVal == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

            while (pAdapter && interfaceCount < maxInterfaces) {
                if (strlen(pAdapter->IpAddressList.IpAddress.String) > 0 &&
                    strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0) {

                    if (strcmp(pAdapter->IpAddressList.IpAddress.String, "127.0.0.1") != 0) {

                        strcpy(interfaces[interfaceCount].ip, pAdapter->IpAddressList.IpAddress.String);
                        strcpy(interfaces[interfaceCount].subnet, pAdapter->IpAddressList.IpMask.String);

                        interfaceCount++;
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }

        free(pAdapterInfo);
    }

    return interfaceCount;
}

void CalculateIPRange(const char* ip, const char* subnetMask,
                     unsigned long* startIP, unsigned long* endIP) {
    struct in_addr ipAddr, maskAddr, networkAddr;

    inet_pton(AF_INET, ip, &ipAddr);
    inet_pton(AF_INET, subnetMask, &maskAddr);

    networkAddr.s_addr = ipAddr.s_addr & maskAddr.s_addr;

    struct in_addr broadcastAddr;
    broadcastAddr.s_addr = networkAddr.s_addr | ~maskAddr.s_addr;

    *startIP = ntohl(networkAddr.s_addr) + 1;
    *endIP = ntohl(broadcastAddr.s_addr) - 1;
}

char* IPToString(unsigned long ip, char* buffer) {
    struct in_addr addr;
    addr.s_addr = htonl(ip);
    inet_ntop(AF_INET, &addr, buffer, 16);
    return buffer;
}

DWORD WINAPI ScanThreadProc(LPVOID lpParam) {
    ScanTask* task = (ScanTask*)lpParam;
    if (!task) return 0;

    for (int i = 0; i < task->count && scanThreadRunning; i++) {
        char ip[16];
        struct in_addr addr;
        addr.s_addr = htonl(task->ipList[i]);
        inet_ntop(AF_INET, &addr, ip, 16);

        if (QuickPing(ip, 100)) {
            BOOL port8888Result = QuickPortCheck(ip, 8888, 150);

            char deviceType[50] = "Device";
            if (QuickPortCheck(ip, 80, 100)) {
                strcpy(deviceType, "Web Server");
            } else if (QuickPortCheck(ip, 445, 100)) {
                strcpy(deviceType, "Windows Host");
            } else if (QuickPortCheck(ip, 22, 100)) {
                strcpy(deviceType, "SSH Host");
            }

            wchar_t wip[16], wdeviceType[50];
            AnsiToUnicode(ip, wip, sizeof(wip)/sizeof(wchar_t));
            AnsiToUnicode(deviceType, wdeviceType, sizeof(wdeviceType)/sizeof(wchar_t));

            EnterCriticalSection(&csDeviceList);

            BOOL exists = FALSE;
            for (int j = 0; j < deviceCount; j++) {
                if (wcscmp(devices[j].ip, wip) == 0) {
                    exists = TRUE;
                    break;
                }
            }

            if (!exists && deviceCount < MAX_DEVICES) {
                wcscpy(devices[deviceCount].ip, wip);
                devices[deviceCount].isAlive = TRUE;
                devices[deviceCount].hasServer = port8888Result;
                wcscpy(devices[deviceCount].deviceType, wdeviceType);
                deviceCount++;
            }

            LeaveCriticalSection(&csDeviceList);

            if (!exists && task->hwnd != NULL) {
                wchar_t line[256];
                if (port8888Result) {
                    swprintf(line, 256, L"✓ %s [%s] - SERVER FOUND", wip, wdeviceType);
                } else {
                    swprintf(line, 256, L"✓ %s [%s] - Online", wip, wdeviceType);
                }
                PostMessage(task->hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(line));
            }
        }
    }

    free(task->ipList);
    free(task);
    return 0;
}

void MultiThreadBatchScan(unsigned long* ipList, int totalCount, HWND hwnd) {
    if (totalCount == 0 || !ipList || !scanThreadRunning) return;

    int threadCount = min(MAX_THREADS, (totalCount + BATCH_SIZE - 1) / BATCH_SIZE);
    if (threadCount == 0) threadCount = 1;

    HANDLE* threads = (HANDLE*)malloc(threadCount * sizeof(HANDLE));
    if (!threads) {
        free(ipList);
        return;
    }

    int ipsPerThread = totalCount / threadCount;
    int remainder = totalCount % threadCount;
    int startIdx = 0;

    for (int i = 0; i < threadCount && scanThreadRunning; i++) {
        int count = ipsPerThread;
        if (i < remainder) count++;

        if (count > 0) {
            ScanTask* task = (ScanTask*)malloc(sizeof(ScanTask));
            if (!task) continue;

            task->ipList = (unsigned long*)malloc(count * sizeof(unsigned long));
            if (!task->ipList) {
                free(task);
                continue;
            }

            task->count = count;
            task->hwnd = hwnd;
            task->threadId = i + 1;

            memcpy(task->ipList, &ipList[startIdx], count * sizeof(unsigned long));
            startIdx += count;

            threads[i] = CreateThread(
                NULL,
                0,
                ScanThreadProc,
                task,
                0,
                NULL
            );
        }
    }

    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);

    for (int i = 0; i < threadCount; i++) {
        if (threads[i]) {
            CloseHandle(threads[i]);
        }
    }

    free(threads);
}

void FastScanIPRangeMultiThread(unsigned long startIP, unsigned long endIP, HWND hwnd) {
    unsigned long rangeSize = endIP - startIP + 1;

    int maxToScan = 100;

    unsigned long* ipList = (unsigned long*)malloc(maxToScan * sizeof(unsigned long));
    if (!ipList) return;

    int idx = 0;

    for (unsigned long ip = startIP; ip < startIP + 10 && idx < maxToScan; ip++) {
        ipList[idx++] = ip;
    }

    unsigned long popular[] = {1, 2, 10, 50, 100, 150, 200, 254};
    unsigned long network = startIP - 1;

    for (int i = 0; i < 8 && idx < maxToScan; i++) {
        if (popular[i] < rangeSize) {
            ipList[idx++] = network + popular[i];
        }
    }

    srand(GetTickCount());
    while (idx < maxToScan && scanThreadRunning) {
        unsigned long randomIP = startIP + (rand() % rangeSize);

        BOOL exists = FALSE;
        for (int j = 0; j < idx; j++) {
            if (ipList[j] == randomIP) {
                exists = TRUE;
                break;
            }
        }

        if (!exists) {
            ipList[idx++] = randomIP;
        }
    }

    if (hwnd != NULL && scanThreadRunning) {
        wchar_t info[256];
        swprintf(info, 256, L"Scanning %d addresses...", idx);
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(info));

        swprintf(info, 256, L"Scanned: 0/%d (0%%)", idx);
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(info));
    }

    if (idx > 0 && scanThreadRunning) {
        MultiThreadBatchScan(ipList, idx, hwnd);
    } else {
        free(ipList);
    }

    if (hwnd != NULL && scanThreadRunning) {
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Network scan completed"));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
    }
}

DWORD WINAPI ScanNetworkThread(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    deviceCount = 0;
    scanThreadRunning = TRUE;

    InitializeCriticalSection(&csDeviceList);
    InitializeCriticalSection(&csConsole);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        if (hwnd != NULL) {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Network initialization failed"));
        }
        scanThreadRunning = FALSE;
        DeleteCriticalSection(&csDeviceList);
        DeleteCriticalSection(&csConsole);
        return 1;
    }
    if (hwnd != NULL) {
        PostMessage(hwnd, WM_USER + 5, 0, 0);
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"=== NETWORK SCANNER ==="));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Using multi-thread scanning"));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"========================"));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Checking localhost..."));
    }

    if (QuickPing("127.0.0.1", 200)) {
        BOOL hasServer = QuickPortCheck("127.0.0.1", 8888, 200);

        if (hwnd != NULL) {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"IP: 127.0.0.1"));
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Type: Localhost"));

            if (hasServer) {
                PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Status: SERVER FOUND"));
            } else {
                PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Status: Online"));
            }
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        }

        if (deviceCount < MAX_DEVICES) {
            wcscpy(devices[deviceCount].ip, L"127.0.0.1");
            devices[deviceCount].isAlive = TRUE;
            devices[deviceCount].hasServer = hasServer;
            wcscpy(devices[deviceCount].deviceType, L"Localhost");
            deviceCount++;
        }
    }

    NetworkInterfaceInfo interfaces[10];
    int interfaceCount = GetNetworkInterfaces(interfaces, 10);

    if (hwnd != NULL) {
        wchar_t infoMsg[256];
        if (interfaceCount > 0) {
            swprintf(infoMsg, 256, L"Found %d network interface(s)", interfaceCount);
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(infoMsg));

            for (int i = 0; i < interfaceCount; i++) {
                wchar_t wip[16], wmask[16];
                AnsiToUnicode(interfaces[i].ip, wip, sizeof(wip)/sizeof(wchar_t));
                AnsiToUnicode(interfaces[i].subnet, wmask, sizeof(wmask)/sizeof(wchar_t));
                swprintf(infoMsg, 256, L"Interface %d: IP=%s, Mask=%s", i+1, wip, wmask);
                PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(infoMsg));
            }
        } else {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"No network interfaces found"));
        }
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Starting network scan..."));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
    }

    for (int i = 0; i < interfaceCount && scanThreadRunning; i++) {
        unsigned long startIP, endIP;
        CalculateIPRange(interfaces[i].ip, interfaces[i].subnet, &startIP, &endIP);

        if (hwnd != NULL && scanThreadRunning) {
            wchar_t networkInfo[256];
            char startIPStr[16], endIPStr[16];
            IPToString(startIP, startIPStr);
            IPToString(endIP, endIPStr);

            wchar_t wstart[16], wend[16];
            AnsiToUnicode(startIPStr, wstart, sizeof(wstart)/sizeof(wchar_t));
            AnsiToUnicode(endIPStr, wend, sizeof(wend)/sizeof(wchar_t));

            swprintf(networkInfo, 256, L"Scanning network %d/%d: %s - %s",
                    i+1, interfaceCount, wstart, wend);
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(networkInfo));
        }

        FastScanIPRangeMultiThread(startIP, endIP, hwnd);

        if (hwnd != NULL && i < interfaceCount - 1 && scanThreadRunning) {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Moving to next network..."));
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        }
    }

    if (scanThreadRunning && hwnd != NULL) {
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"========================"));

        if (deviceCount > 0) {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Found devices:"));

            for (int i = 0; i < deviceCount - 1; i++) {
                for (int j = i + 1; j < deviceCount; j++) {
                    if (wcscmp(devices[i].ip, devices[j].ip) > 0) {
                        NetworkDevice temp = devices[i];
                        devices[i] = devices[j];
                        devices[j] = temp;
                    }
                }
            }

            for (int i = 0; i < deviceCount; i++) {
                wchar_t deviceInfo[256];
                if (devices[i].hasServer) {
                    swprintf(deviceInfo, 256, L"✓ %s [%s] - SERVER",
                             devices[i].ip, devices[i].deviceType);
                } else {
                    swprintf(deviceInfo, 256, L"  %s [%s]",
                             devices[i].ip, devices[i].deviceType);
                }
                PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(deviceInfo));
            }
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L""));
        }

        wchar_t summary[256];
        int serverCount = 0;
        for (int i = 0; i < deviceCount; i++) {
            if (devices[i].hasServer) serverCount++;
        }

        swprintf(summary, 256, L"Scan complete: %d devices found", deviceCount);
        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(summary));

        if (serverCount > 0) {
            swprintf(summary, 256, L"Servers: %d on port 8888", serverCount);
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(summary));
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"Use 'Connect to Server' button"));
        } else {
            PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"No servers found on port 8888"));
        }

        PostMessage(hwnd, WM_USER + 4, 0, (LPARAM)_wcsdup(L"========================"));
        PostMessage(hwnd, WM_USER + 2, 0, (LPARAM)L"Scan completed");
    }

    WSACleanup();
    DeleteCriticalSection(&csDeviceList);
    DeleteCriticalSection(&csConsole);
    scanThreadRunning = FALSE;
    return 0;
}

void StopScanning() {
    scanThreadRunning = FALSE;

    if (hScanThread) {
        WaitForSingleObject(hScanThread, 3000);
        CloseHandle(hScanThread);
        hScanThread = NULL;
    }
}