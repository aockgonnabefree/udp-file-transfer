#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "packet.h"
#include "protocol.h"

#define IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

#define ARQ_STOP_AND_WAIT 1

#include <string.h>

// void send_file(int sockfd, const char *filename, struct sockaddr_in *client_addr, socklen_t client_len);

int main(){
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
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);

    //
    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[ERROR] bind error");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("Server is running. Waiting for SYN packet. (CTRL+C to exit)\n");
        if (server_handle_syn(server_sockfd, &client_addr, client_len) == -1) {
            printf("[ERROR] Connection Establishment failed\n");
            // continue; // Wait for the next client
        }
    }

    // Close UDP socket
    close(server_sockfd);
    return 0;
}

// Send file from ../example/ directory to client
// void send_file(int sockfd, const char *filename, struct sockaddr_in *client_addr, socklen_t client_len) {
//     char path[256];
//     snprintf(path, sizeof(path), "../example/%s", filename);
//     FILE *fp = fopen(path, "r");
//     if (fp == NULL) {
//         printf("[ERROR] file not found: %s\n", path);
//         // Optionally send error message to client
//         return;
//     }
//     char file_buffer[BUFFER_SIZE];
//     int n;
//     while ((n = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
//         sendto(sockfd, file_buffer, n, 0, (struct sockaddr *)client_addr, client_len);
//     }
//     fclose(fp);
//     printf("[SUCCESS] File sent to client\n");
// }