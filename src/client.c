#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "packet.h"
#include "protocol.h"

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 4) {
        printf("Usage: %s <server_ip> <port> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *filename = argv[3];

    // Validate port
    if (port <= 0 || port > 65535) {
        printf("[CLIENT][ERROR] Invalid port number. Must be between 1-65535\n");
        exit(EXIT_FAILURE);
    }

    // Define variables
    int server_sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);

    // Create an UDP socket
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        printf("[CLIENT][ERROR] socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("[CLIENT][ERROR] Invalid server IP address\n");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[CLIENT] Connecting to %s:%d\n", server_ip, port);
    printf("[CLIENT] Requesting file: %s\n", filename);

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