#ifndef SCANNER_H
#define SCANNER_H

#include <windows.h>

#include "config.h"

#define MAX_DEVICES 1000

void InitScanner();
void CleanupScanner();

DWORD WINAPI ScanNetworkThread(LPVOID lpParam);
void StopScanning();
BOOL StartNetworkScan(HWND hwnd);
int GetScannedDevices(NetworkDevice** deviceList);

void AnsiToUnicode(const char* ansi, wchar_t* unicode, int maxLen);

#endif