#include "header/network_utils.h"

void AnsiToUnicode(const char* ansi, wchar_t* unicode, int maxLen) {
    if (ansi && unicode && maxLen > 0) {
        MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, maxLen);
        unicode[maxLen - 1] = L'\0';
    }
}

void UnicodeToAnsi(const wchar_t* unicode, char* ansi, int maxLen) {
    if (unicode && ansi && maxLen > 0) {
        WideCharToMultiByte(CP_ACP, 0, unicode, -1, ansi, maxLen, NULL, NULL);
        ansi[maxLen - 1] = '\0';
    }
}

BOOL PingDevice(const char* ip) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return FALSE;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return FALSE;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int timeout = 500;// 500ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    BOOL isAlive = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);

    closesocket(sock);
    WSACleanup();

    return isAlive;
}

void GetLocalIP(char* ipBuffer) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return;

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host != NULL && host->h_addr_list[0] != NULL) {
            struct in_addr addr;
            memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
            strcpy(ipBuffer, inet_ntoa(addr));
        }
    }

    WSACleanup();
}

BOOL CheckServerOnDevice(const wchar_t* wip) {
    char ip[MAX_IP_LENGTH];
    UnicodeToAnsi(wip, ip, sizeof(ip));

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return FALSE;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return FALSE;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip, &serverAddr.sin_addr) != 1) {
        closesocket(sock);
        WSACleanup();
        return FALSE;
    }

    int timeout = 500;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    BOOL hasServer = (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0);

    closesocket(sock);
    WSACleanup();

    return hasServer;
}