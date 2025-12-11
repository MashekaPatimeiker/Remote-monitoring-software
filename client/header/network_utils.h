#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include "config.h"

void AnsiToUnicode(const char* ansi, wchar_t* unicode, int maxLen);
void UnicodeToAnsi(const wchar_t* unicode, char* ansi, int maxLen);

BOOL PingDevice(const char* ip);
void GetLocalIP(char* ipBuffer);
BOOL CheckServerOnDevice(const wchar_t* wip);

#endif