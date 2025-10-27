#include "socketutil.h"

struct AcceptedSocket {
    socket_t acceptedSocketFd;
    struct sockaddr_in clientAddress;
};

struct ClientNode {
    struct AcceptedSocket* client;
    struct ClientNode* next;
};

static pthread_mutex_t g_clientsMutex = PTHREAD_MUTEX_INITIALIZER;
static struct ClientNode* g_clientsHead = NULL;

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

static bool add_client(struct AcceptedSocket* client)
{
    struct ClientNode* node = (struct ClientNode*)malloc(sizeof(*node));
    if (!node) {
        fprintf(stderr, "malloc failed while tracking client\n");
        return false;
    }

    node->client = client;

    pthread_mutex_lock(&g_clientsMutex);
    node->next = g_clientsHead;
    g_clientsHead = node;
    pthread_mutex_unlock(&g_clientsMutex);

    char ipStr[INET_ADDRSTRLEN] = "unknown";
    if (!inet_ntop(AF_INET, &client->clientAddress.sin_addr, ipStr, sizeof(ipStr))) {
        strncpy(ipStr, "unknown", sizeof(ipStr));
        ipStr[sizeof(ipStr) - 1] = '\0';
    }
    printf("Client connected: %s:%d\n", ipStr, ntohs(client->clientAddress.sin_port));
    return true;
}

static void remove_client(struct AcceptedSocket* client)
{
    pthread_mutex_lock(&g_clientsMutex);

    struct ClientNode** current = &g_clientsHead;
    while (*current) {
        if ((*current)->client == client) {
            struct ClientNode* toRemove = *current;
            *current = toRemove->next;
            free(toRemove);
            break;
        }
        current = &(*current)->next;
    }

    pthread_mutex_unlock(&g_clientsMutex);
}

static void broadcast_message(struct AcceptedSocket* sender, const char* data, size_t length)
{
    if (!sender || !data || length == 0) {
        return;
    }

    char senderIp[INET_ADDRSTRLEN] = "unknown";
    if (!inet_ntop(AF_INET, &sender->clientAddress.sin_addr, senderIp, sizeof(senderIp))) {
        strncpy(senderIp, "unknown", sizeof(senderIp));
        senderIp[sizeof(senderIp) - 1] = '\0';
    }
    int senderPort = ntohs(sender->clientAddress.sin_port);

    char composedMessage[BUFFER_SIZE + 64];
    int written = snprintf(composedMessage, sizeof(composedMessage), "[%s:%d] %.*s",
        senderIp, senderPort, (int)length, data);
    if (written < 0) {
        return;
    }

    size_t messageLength = (size_t)written;
    if (messageLength >= sizeof(composedMessage)) {
        messageLength = sizeof(composedMessage) - 1;
        composedMessage[messageLength] = '\0';
    }

    pthread_mutex_lock(&g_clientsMutex);

    struct ClientNode* node = g_clientsHead;
    while (node) {
        if (node->client && node->client != sender && node->client->acceptedSocketFd != INVALID_SOCKET) {
            size_t totalSent = 0;
            while (totalSent < messageLength) {
                int sent = send(node->client->acceptedSocketFd,
                    composedMessage + totalSent,
                    (int)(messageLength - totalSent), 0);
                if (sent == SOCKET_ERROR) {
                    print_last_error("broadcast send");
                    break;
                }
                totalSent += (size_t)sent;
            }
        }
        node = node->next;
    }

    pthread_mutex_unlock(&g_clientsMutex);
}

static void* recv_data(void* arg)
{
    struct AcceptedSocket* clientSocket = (struct AcceptedSocket*)arg;
    if (!clientSocket) {
        return NULL;
    }

    char clientIp[INET_ADDRSTRLEN] = "unknown";
    if (!inet_ntop(AF_INET, &clientSocket->clientAddress.sin_addr, clientIp, sizeof(clientIp))) {
        strncpy(clientIp, "unknown", sizeof(clientIp));
        clientIp[sizeof(clientIp) - 1] = '\0';
    }
    int clientPort = ntohs(clientSocket->clientAddress.sin_port);

    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket->acceptedSocketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            printf("Received from %s:%d -> %s\n", clientIp, clientPort, buffer);
            broadcast_message(clientSocket, buffer, (size_t)bytesReceived);
        } else if (bytesReceived == 0) {
            printf("Client disconnected: %s:%d\n", clientIp, clientPort);
            break;
        } else {
            print_last_error("recv");
            break;
        }
    }

    remove_client(clientSocket);
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

        if (!add_client(clientSocket)) {
            close_client_socket(clientSocket);
            continue;
        }

        pthread_t threadId;
        int threadErr = pthread_create(&threadId, NULL, recv_data, clientSocket);
        if (threadErr != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", threadErr);
            remove_client(clientSocket);
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
