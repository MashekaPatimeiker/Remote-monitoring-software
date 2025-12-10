#ifndef SCANNER_H
#define SCANNER_H

#include <windows.h>

#include "config.h"

#define MAX_DEVICES 1000

// Инициализация/очистка
void InitScanner();
void CleanupScanner();

// Основные функции
DWORD WINAPI ScanNetworkThread(LPVOID lpParam);
void StopScanning();
BOOL StartNetworkScan(HWND hwnd);
int GetScannedDevices(NetworkDevice** deviceList);

// Вспомогательные функции (должны быть в network_utils.h)
void AnsiToUnicode(const char* ansi, wchar_t* unicode, int maxLen);

#endif