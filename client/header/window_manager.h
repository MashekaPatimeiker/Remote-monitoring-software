#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "config.h"

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void HandleCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
void HandleNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
void HandleSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

#endif