#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "packet.h"
#include "protocol.h"

#define BUFFER_SIZE 1024

#define ARQ_STOP_AND_WAIT 1

#include <string.h>

int main(int argc, char *argv[]){
    // Check command-line arguments
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse port from command-line
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("[ERROR] Invalid port number. Must be between 1-65535\n");
        exit(EXIT_FAILURE);
    }

    // Define variables
    int server_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create an UDP socket
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        printf("[ERROR] socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[ERROR] bind error");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Server started on port %d\n", port);

    while(1) {
        connection_t conn;
        printf("[SERVER] Waiting for SYN packet. (CTRL+C to exit)\n");
        if (server_handle_syn(server_sockfd, &client_addr, client_len, &conn) == 0) {
            send_file(server_sockfd, &client_addr, client_len, &conn);
        } else {
            printf("[ERROR] Connection Establishment failed\n");
        }

        struct timeval tv_reset;
        tv_reset.tv_sec = 0;
        tv_reset.tv_usec = 0;
        setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_reset, sizeof tv_reset);
    }

    // Close UDP socket
    close(server_sockfd);
    return 0;
}