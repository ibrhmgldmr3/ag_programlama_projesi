#include <socketutil.h>

static void* receive_messages(void* arg)
{
    socket_t sockfd = *(socket_t*)arg;
    char buffer[BUFFER_SIZE];

    while (true)
    {
        int received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received > 0)
        {
            buffer[received] = '\0';
            printf("\nMessage from server: %s\n", buffer);
            printf("Enter message to send(type \"exit\" to exit):\n");
        }
        else if (received == 0)
        {
            printf("\nServer closed the connection.\n");
            break;
        }
        else
        {
            print_last_error("recv");
            break;
        }
    }

    return NULL;
}

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
    
    pthread_t receiverThread;
    int threadErr = pthread_create(&receiverThread, NULL, receive_messages, &socketFD);
    if (threadErr != 0)
    {
        fprintf(stderr, "Failed to create receiver thread: %d\n", threadErr);
        free(addrinfo_result);
        closesocket(socketFD);
        WSACleanup();
        return EXIT_FAILURE;
    }

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
        // #can you send data and the info of the client socket
        printf("Sending data to %s:%d\n", ip, port);
        printf("Client socket info:\n");
        print_socket_info(socketFD);

        size_t total_sent = 0;
        while (total_sent < charCount)
        {
            int amountWasSent = send(socketFD, line + total_sent, (int)(charCount - total_sent), 0);
            if (amountWasSent == SOCKET_ERROR)
            {
                print_last_error("send");
                free(addrinfo_result);
                shutdown(socketFD, SD_BOTH);
                pthread_join(receiverThread, NULL);
                closesocket(socketFD);
                WSACleanup();
                return EXIT_FAILURE;
            }
            total_sent += (size_t)amountWasSent;
        }

    }
    

    printf("Exiting...\n");

    shutdown(socketFD, SD_BOTH);
    pthread_join(receiverThread, NULL);

    printf("\n");
    free(addrinfo_result);
    closesocket(socketFD);
    WSACleanup();
    return EXIT_SUCCESS;
}
