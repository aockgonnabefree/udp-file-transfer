#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define PORT 8080
#define SIZE 1024

#include <string.h>

// Send file from ../example/ directory to client
void send_file(int sockfd, const char *filename, struct sockaddr_in *client_addr, socklen_t client_len) {
    char path[256];
    snprintf(path, sizeof(path), "../example/%s", filename);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("[ERROR] file not found: %s\n", path);
        // Optionally send error message to client
        return;
    }
    char file_buffer[SIZE];
    int n;
    while ((n = fread(file_buffer, 1, SIZE, fp)) > 0) {
        sendto(sockfd, file_buffer, n, 0, (struct sockaddr *)client_addr, client_len);
    }
    fclose(fp);
    printf("[SUCCESS] File sent to client\n");
}

int main()
{
    // Define variables
    int server_sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[SIZE];

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

    // Receive filename from client
    socklen_t client_len = sizeof(client_addr);
    int recv_len = recvfrom(server_sockfd, buffer, SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
    if (recv_len < 0) {
        printf("[ERROR] failed to receive filename\n");
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0'; // Ensure null-terminated
    printf("[INFO] Requested file: %s\n", buffer);

    send_file(server_sockfd, buffer, &client_addr, client_len);

    // Close UDP socket
    close(server_sockfd);

    return 0;
}