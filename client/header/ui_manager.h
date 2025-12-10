#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "config.h"

// UI handles
extern HWND hEditIP, hConnectBtn, hListBox, hStatus, hStats, hRefreshBtn, hAutoCheck;
extern HWND hScanBtn, hDeviceList, hConnectToDeviceBtn, hTabControl, hScanListBox;  // Добавили hScanListBox
extern wchar_t serverIP[MAX_IP_LENGTH];
extern BOOL autoRefresh;
extern UINT_PTR refreshTimer;

// UI functions
void CreateUIElements(HWND hwnd);
void UpdateConnectionStatus(const wchar_t* status);
void AddScanResult(const wchar_t* result);
void ClearScanResults();
void LayoutUI(HWND hwnd, int width, int height);

#endif