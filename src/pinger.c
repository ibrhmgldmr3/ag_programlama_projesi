// client.c
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

int main(void) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in addr;
    char buf[256];
    int len;

    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5025);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("connect error\n");
        return 1;
    }

    send(sock, "*IDN?\n", 6, 0);
    len = recv(sock, buf, sizeof(buf)-1, 0);
    if (len > 0) {
        buf[len] = '\0';
        printf("Yanit: %s", buf);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
