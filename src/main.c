// dummy_server.c  (UTF-8 kaydedin)
#define WIN32_LEAN_AND_MEAN            // windows.h şişmesini azaltır
#define _WIN32_WINNT 0x0601            // expose inet_pton/getaddrinfo on MinGW
#include <winsock2.h>                  // Winsock temel fonksiyonlar
#include <ws2tcpip.h>                  // inet_pton vb.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")     // VS için link yönergesi

int main(void) {
    // --- Winsock başlat ---
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // --- Sunucu soketi oluştur ---
    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) {
        printf("socket() failed\n");
        WSACleanup();
        return 1;
    }

    // --- Hızlı yeniden başlatma için SO_REUSEADDR (opsiyonel) ---
    {
        int yes = 1;
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    }

    // --- 127.0.0.1:5025 adresini hazırla ---
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(5025);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        printf("inet_addr() failed\n");
        closesocket(srv);
        WSACleanup();
        return 1;
    }

    // --- Bağla (bind) ---
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("bind() failed (port meşgul olabilir)\n");
        closesocket(srv);
        WSACleanup();
        return 1;
    }

    // --- Dinlemeye başla ---
    if (listen(srv, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen() failed\n");
        closesocket(srv);
        WSACleanup();
        return 1;
    }
    printf("Dummy SCPI server dinliyor: 127.0.0.1:5025\n");

    // --- Basit kabul döngüsü (tek iş parçacığı, bir istemciyi sırayla işler) ---
    for (;;) {
        struct sockaddr_in cli;
        int clen = (int)sizeof(cli);
        SOCKET c = accept(srv, (struct sockaddr*)&cli, &clen);
        if (c == INVALID_SOCKET) {
            printf("accept() failed\n");
            break;
        }
        printf("Baglanti geldi\n");

        // --- Basit satır bazlı okuma: \n’e kadar karakter topla ---
        enum { LINE_MAX = 1024 };
        char line[LINE_MAX];
        int  len = 0;
        bool done = false;

        while (!done) {
            char ch;
            int r = recv(c, &ch, 1, 0);
            if (r == 0) {
                printf("Client kapatti\n");
                done = true; // bağlantı kapandı
                break;
            }
            if (r < 0) {
                printf("recv() hata\n");
                done = true;
                break;
            }
            if (ch == '\n') {
                // satır bitti
                break;
            }
            if (ch != '\r') {
                if (len < LINE_MAX - 1) {
                    line[len++] = ch;
                } else {
                    // tampon doldu; pratikte burada flush/ignore yapılır
                    // biz sadece kalanları yoksayıp devam ediyoruz
                }
            }
            // Not: gerçek server’da buffer sınırı/timeout eklenmeli
        }
        line[len] = '\0';

        printf("Komut: %s\n", line);

        // --- SCPI: *IDN? gelirse kimlik döndür, aksi halde hata ---
        if (len > 0) {
            if (strcmp(line, "*IDN?") == 0) {
                const char* idn = "FAKE,MODEL-1234,SN0001,FW1.0\n";
                send(c, idn, (int)strlen(idn), 0);
            } else {
                const char* err = "-113,\"Undefined header\"\n";
                send(c, err, (int)strlen(err), 0);
            }
        }

        // basitlik için her komuttan sonra kapatıyoruz
        closesocket(c);
    }

    closesocket(srv);
    WSACleanup();
    return 0;
}   
