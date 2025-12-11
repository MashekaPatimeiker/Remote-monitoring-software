#ifndef CONFIG_H
#define CONFIG_H

#define PORT 8888
#define BUFFER_SIZE 65536
#define MAX_PROCESSES 512
#define MAX_PROCESS_NAME 256

#define ID_LISTBOX         1001
#define ID_STATUS          1002
#define ID_BUTTON_START    1003
#define ID_REFRESH         1004

typedef struct {
    char name[256];
    DWORD pid;
    DWORD memory;
    DWORD threadCount;
} ProcessInfo;

extern BOOL serverRunning;
extern SOCKET serverSocket;
extern HANDLE hServerThread;

#endif