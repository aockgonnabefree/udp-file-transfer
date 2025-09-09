#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

int send_file(int sockfd, const char *filename, struct sockaddr_in *dest_addr, socklen_t dest_len);

int receive_file(int server_sockdf, const char *filename);



#endif