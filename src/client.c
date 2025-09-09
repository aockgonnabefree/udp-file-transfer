#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define IP "127.0.0.1"
#define PORT 8080

// TODO: add more params
void send_file() {

}

int main() {

    // 
    int server_sockfd;
    struct sockaddr_in server_addr;
    char *filename = "example.c";
    FILE *fp = fopen(filename, "r");

    // 
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (server_sockfd < 0) {
        printf("[ERROR] socket error");
        exit(EXIT_FAILURE);
    }

    // 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    
    // 
    if (fp == NULL) {
        printf("[ERROR] reading the file");
        exit(EXIT_FAILURE);
    }

    // Sending the file
    send_file();

    
    

    return 0;
}