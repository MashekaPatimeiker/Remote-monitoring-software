#ifndef CONFIG_H
#define CONFIG_H

#define PORT 8888
// config.h
#define BUFFER_SIZE 65536  // Увеличено с 8192 до 65536 (64KB)
#define MAX_PROCESSES 512
#define MAX_PROCESS_NAME 256

// Control IDs
#define ID_LISTBOX         1001
#define ID_STATUS          1002
#define ID_BUTTON_START    1003
#define ID_REFRESH         1004

// Process information structure
typedef struct {
    char name[256];
    DWORD pid;
    DWORD memory;
    DWORD threadCount;
} ProcessInfo;

// Глобальные переменные (объявления)
extern BOOL serverRunning;
extern SOCKET serverSocket;
extern HANDLE hServerThread;

#endif