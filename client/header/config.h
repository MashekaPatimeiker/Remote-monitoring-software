#ifndef CONFIG_H
#define CONFIG_H

// Windows headers
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <time.h>
#include <iphlpapi.h>
#include <locale.h>

// Library linking
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Constants
#define PORT 8888
#define BUFFER_SIZE 65536
#define MAX_DEVICES 100
#define MAX_IP_LENGTH 16
#define MAX_HOSTNAME_LENGTH 256

// Control IDs
#define ID_EDIT_IP         1001
#define ID_BUTTON_CONN     1002
#define ID_LISTBOX         1003
#define ID_STATUS          1004
#define ID_STATS           1005
#define ID_REFRESH         1006
#define ID_AUTO            1007
#define ID_SCAN_NETWORK    1008
#define ID_CONNECT_TO_DEV  1010
#define ID_TAB_CONTROL     1011
#define ID_SCAN_LISTBOX    1012

#define MAX_DEVICES 1000

typedef struct {
    wchar_t ip[16];
    BOOL isAlive;
    BOOL hasServer;
    wchar_t deviceType[50];
    wchar_t mac[18];  // MAC адрес (опционально)
    int openPorts[20]; // Список открытых портов
} NetworkDevice;

// Структура для безопасной передачи сообщений между потоками
typedef struct {
    HWND hwnd;
    wchar_t message[1024];
} ScanMessage;

// Global variables declarations
extern HWND hEditIP, hConnectBtn, hListBox, hStatus, hStats, hRefreshBtn, hAutoCheck;
extern HWND hScanBtn, hDeviceList, hConnectToDeviceBtn, hTabControl, hScanListBox;
extern wchar_t serverIP[MAX_IP_LENGTH];
extern BOOL autoRefresh;
extern UINT_PTR refreshTimer;
extern NetworkDevice devices[MAX_DEVICES];
extern int deviceCount;
extern HANDLE hScanThread;
extern BOOL scanThreadRunning;

// Для обратной совместимости с кодом, использующим старые имена
#ifndef hResultText
#define hResultText hScanListBox
#endif

#endif // CONFIG_H