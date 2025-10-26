#include <socketutil.h>

int main()
{

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    
    const char *ip = "127.0.0.1";
    int port = 2000;

    struct sockaddr_in* addrinfo_result = createIPv4Address(ip, port);
    if (!addrinfo_result) {
        fprintf(stderr, "Failed to create IPv4 address\n");
        WSACleanup();
        return EXIT_FAILURE;
    }

    socket_t socketFD = create_socket();
    if (socketFD == INVALID_SOCKET) {
        free(addrinfo_result);
        WSACleanup();
        return EXIT_FAILURE;
    }

    int result = connect(socketFD, (struct sockaddr*)addrinfo_result, sizeof(struct sockaddr_in));
    if (result == SOCKET_ERROR) {
        print_last_error("connect");
        free(addrinfo_result);
        closesocket(socketFD);
        WSACleanup();
        return EXIT_FAILURE;
    }
    printf("Connected to %s:%d\n", ip, port);
    

    char line[BUFFER_SIZE];
    printf("Enter message to send(type \"exit\" to exit):\n");
    while(1)
    {
        if (!fgets(line, sizeof(line), stdin))
        {
            break;
        }
        size_t charCount = strlen(line);
        if (charCount > 0 && line[charCount - 1] == '\n')
        {
            line[charCount - 1] = '\0';
            --charCount;
        }
        if (charCount == 0)
        {
            continue;
        }
        if(strcmp(line,"exit")==0)
        {
            break;
        }
        size_t total_sent = 0;
        while (total_sent < charCount)
        {
            int amountWasSent = send(socketFD, line + total_sent, (int)(charCount - total_sent), 0);
            if (amountWasSent == SOCKET_ERROR)
            {
                print_last_error("send");
                free(addrinfo_result);
                closesocket(socketFD);
                WSACleanup();
                return EXIT_FAILURE;
            }
            total_sent += (size_t)amountWasSent;
        }

    }
    

    printf("Exiting...\n");

    printf("\n");
    free(addrinfo_result);
    closesocket(socketFD);
    WSACleanup();
    return EXIT_SUCCESS;
}



