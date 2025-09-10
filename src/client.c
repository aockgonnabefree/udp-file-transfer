#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "packet.h"
#include "protocol.h"

#define IP "127.0.0.1"
#define PORT 8080

int main()
{
    // Define variables
    int server_sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    
    // char *filename = "example.jpeg";
    char *filename = "Chapter_1_v9.0.pptx";

    // Create an UDP socket
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        printf("[CLIENT][ERROR] socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    // Initiate Connection by 3-way handshake
    connection_t conn;
    if (client_perform_handshake(server_sockfd, &server_addr, server_len, filename, &conn) < 0) {
        printf("[CLIENT][ERROR] connection established failed\n");
        exit(EXIT_FAILURE);
    }

    receive_file(server_sockfd, filename, &conn);

    // Close UDP socket
    close(server_sockfd);

    return 0;
}