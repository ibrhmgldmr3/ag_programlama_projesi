#include "socketutil.h"

void print_last_error(const char *label)
{
    fprintf(stderr, "%s failed with error: %d\n", label, WSAGetLastError());
}

void print_socket_info(socket_t sockfd)
{
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    
    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) == 0) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        printf("  Local address: %s:%d\n", ip_str, ntohs(addr.sin_port));
    } else {
        print_last_error("getsockname");
    }
    
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == 0) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        printf("  Remote address: %s:%d\n", ip_str, ntohs(addr.sin_port));
    } else {
        print_last_error("getpeername");
    }
}

socket_t create_socket(void)
{
    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        print_last_error("socket");
        return INVALID_SOCKET;
    }
    return sock;
}
int createIPv4Adress_getaddrinfo(const char *hostname, const char *port, struct addrinfo **result)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addrinfo_result = NULL;
    int getaddrinfo_result = getaddrinfo(hostname, port, &hints, &addrinfo_result);
    if (getaddrinfo_result != 0)
    {
        fprintf(stderr, "getaddrinfo failed: %d\n", getaddrinfo_result);
        WSACleanup();
        return EXIT_FAILURE;
    }
    *result = addrinfo_result;
    return 0;
}
struct sockaddr_in* createIPv4Address(const char* ip, int port){
    struct sockaddr_in* addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    if (!addr)
    {
        fprintf(stderr, "Failed to allocate sockaddr_in\n");
        return NULL;
    }
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if(strlen(ip) ==0)
        addr->sin_addr.s_addr = INADDR_ANY;
    else
        addr->sin_addr.s_addr = inet_addr(ip);
    return addr;
}

void clean_and_exit(struct addrinfo* addrinfo_result, struct sockaddr_in* sockaddr_result, socket_t sockfd, int exit_code)
{
    if (addrinfo_result)
    {
        freeaddrinfo(addrinfo_result);
    }
    if (sockaddr_result)
    {
        free(sockaddr_result);
    }
    if (sockfd != INVALID_SOCKET)
    {
        closesocket(sockfd);
    }
    WSACleanup();
    exit(exit_code);
}
