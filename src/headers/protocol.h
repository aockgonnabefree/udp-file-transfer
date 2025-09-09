#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

// File Transfer
int send_file(int sockfd, const char *filename, struct sockaddr_in *dest_addr, socklen_t dest_len);
int receive_file(int server_sockdf, const char *filename);

// 3-way handshake
int client_perform_handshake(int sockfd, struct sockaddr_in *server_addr, socklen_t server_len, char *filename);
int server_handle_syn(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len);

#endif