#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <windows.h>
#include <commctrl.h>
#include "config.h"

// Global UI handles
extern HWND hListBox, hStatus, hStartBtn, hRefreshBtn;

// UI functions
void CreateUIElements(HWND hwnd);
void UpdateProcessList();
void UpdateStatus(const char* status);
void LayoutUI(HWND hwnd, int width, int height);

#endif