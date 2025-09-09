#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "packet.h"
#include "protocol.h"

#define IP "127.0.0.1"
#define PORT 8080

// void receive_file(int server_sockfd, const char *filename) {
//     FILE *fp = fopen(filename, "w");
//     if (fp == NULL) {
//         printf("[ERROR] cannot create file: %s\n", filename);
//         close(server_sockfd);
//         exit(EXIT_FAILURE);
//     }
//     char buffer[1024];
//     struct sockaddr_in from_addr;
//     socklen_t from_len = sizeof(from_addr);
//     int n;
//     // Receive until server stops sending (UDP: one-shot, so we use timeout or break on <1024 bytes)
//     while (1) {
//         n = recvfrom(server_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &from_len);
//         if (n <= 0) break;
//         fwrite(buffer, 1, n, fp);
//         if (n < sizeof(buffer)) break; // last packet
//     }
//     fclose(fp);
//     printf("[SUCCESS] File received and saved as %s\n", filename);
// }

int main()
{
    // Define variables
    int server_sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    
    char *filename = "example.jpeg";

    // Create an UDP socket
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sockfd < 0)
    {
        printf("[ERROR] socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    // Initiate Connection by 3-way handshake
    client_perform_handshake(server_sockfd, &server_addr, server_len, filename);

    // Send filename to server
    // sendto(server_sockfd, filename, strlen(filename) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Receive file data from server and save to file
    // receive_file(server_sockfd, filename);

    // Close UDP socket
    close(server_sockfd);

    return 0;
}