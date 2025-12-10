#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <winsock2.h>
#include <windows.h>
#include "config.h"

// Global server variables
extern HANDLE hServerThread;
extern BOOL serverRunning;
extern SOCKET serverSocket;

DWORD WINAPI ServerThread(LPVOID lpParam);
void StopServer();
BOOL IsServerRunning();

#endif