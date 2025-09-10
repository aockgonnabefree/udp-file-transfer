#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

// type สำหรับalgorithm ที่ server เลือกใช้ในการส่งไฟล์
typedef enum {
    STOP_AND_WAIT,
    GO_BACK_N,
    SELECTIVE_REPEAT
}algo_t;

// struct สำหรับการ maintain connection state ของ server หลังจาก handshake กับ client
typedef struct {
    int seq_num;
    int ack_num;
    int window_size;
    algo_t algo;
    char filename[256];
} connection_t;

// File Transfer
int send_file(int sockfd, struct sockaddr_in *dest_addr, socklen_t dest_len, connection_t *conn);
int receive_file(int server_sockdf, const char *filename, connection_t *conn);

// 3-way handshake
int client_perform_handshake(int sockfd, struct sockaddr_in *server_addr, socklen_t server_len, char *filename, connection_t *conn);
int server_handle_syn(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, connection_t *conn);

#endif