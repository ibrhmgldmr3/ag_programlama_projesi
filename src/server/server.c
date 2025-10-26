#include "socketutil.h"

struct AcceptedSocket {
    socket_t acceptedSocketFd;
    struct sockaddr_in clientAddress;
};

static struct AcceptedSocket* acceptIncomingConnection(socket_t serverSocketFD)
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    socket_t acceptResult = accept(serverSocketFD, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (acceptResult == INVALID_SOCKET) {
        print_last_error("accept");
        return NULL;
    }

    struct AcceptedSocket* acceptedSocket = (struct AcceptedSocket*)malloc(sizeof(*acceptedSocket));
    if (!acceptedSocket) {
        fprintf(stderr, "malloc failed while accepting client\n");
        closesocket(acceptResult);
        return NULL;
    }

    acceptedSocket->acceptedSocketFd = acceptResult;
    acceptedSocket->clientAddress = clientAddr;
    return acceptedSocket;
}

static void close_client_socket(struct AcceptedSocket* clientSocket)
{
    if (!clientSocket) {
        return;
    }

    if (clientSocket->acceptedSocketFd != INVALID_SOCKET) {
        closesocket(clientSocket->acceptedSocketFd);
    }
    free(clientSocket);
}

static void* recv_data(void* arg)
{
    struct AcceptedSocket* clientSocket = (struct AcceptedSocket*)arg;
    if (!clientSocket) {
        return NULL;
    }

    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket->acceptedSocketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            printf("Received data: %s\n", buffer);
        } else if (bytesReceived == 0) {
            printf("Client disconnected\n");
            break;
        } else {
            print_last_error("recv");
            break;
        }
    }

    close_client_socket(clientSocket);
    return NULL;
}

int startGettingIncomingConnections(socket_t serverSocketFD)
{
    while (true) {
        struct AcceptedSocket* clientSocket = acceptIncomingConnection(serverSocketFD);
        if (!clientSocket) {
            continue;
        }

        pthread_t threadId;
        int threadErr = pthread_create(&threadId, NULL, recv_data, clientSocket);
        if (threadErr != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", threadErr);
            close_client_socket(clientSocket);
            return threadErr;
        }
        pthread_detach(threadId);
    }

    return 0;
}

int main(void)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return EXIT_FAILURE;
    }

    socket_t serverSocketFD = create_socket();
    if (serverSocketFD == INVALID_SOCKET) {
        WSACleanup();
        return EXIT_FAILURE;
    }

    struct sockaddr_in* serverAddr = createIPv4Address("", 2000);
    if (!serverAddr) {
        closesocket(serverSocketFD);
        WSACleanup();
        return EXIT_FAILURE;
    }

    int bindResult = bind(serverSocketFD, (struct sockaddr*)serverAddr, sizeof(struct sockaddr_in));
    if (bindResult == SOCKET_ERROR) {
        clean_and_exit(NULL, serverAddr, serverSocketFD, EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", 2000);
    int listenResult = listen(serverSocketFD, 10);
    if (listenResult == SOCKET_ERROR) {
        clean_and_exit(NULL, serverAddr, serverSocketFD, EXIT_FAILURE);
    }

    int acceptResult = startGettingIncomingConnections(serverSocketFD);
    if (acceptResult != 0) {
        clean_and_exit(NULL, serverAddr, serverSocketFD, EXIT_FAILURE);
    }

    clean_and_exit(NULL, serverAddr, serverSocketFD, EXIT_SUCCESS);
    return EXIT_SUCCESS;
}
