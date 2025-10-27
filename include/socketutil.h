#ifndef SOCKETUTIL_H
#define SOCKETUTIL_H

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h> 
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif


typedef SOCKET socket_t;

#define BUFFER_SIZE 4096
socket_t create_socket(void);
int createIPv4Adress_getaddrinfo(const char *hostname, const char *port, struct addrinfo **result);
struct sockaddr_in* createIPv4Address(const char* ip, int port);
void print_last_error(const char *label);
void print_socket_info(socket_t sockfd);
void clean_and_exit(struct addrinfo* addrinfo_result, struct sockaddr_in* sockaddr_result, socket_t sockfd, int exit_code);
#endif // SOCKETUTIL_H
