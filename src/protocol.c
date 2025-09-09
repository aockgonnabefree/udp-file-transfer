#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "packet.h"

#define IP "127.0.0.1"
#define PORT 8080

#define BUFFER_SIZE 1024

int send_file(int sockfd, const char *filename, struct sockaddr_in *dest_addr, socklen_t dest_len) {
    char path[256];
    snprintf(path, sizeof(path), "../example/%s", filename);
    FILE *fp = fopen(path, "r");
    
    // Handle in case File not found
    if (fp == NULL) {
        printf("[ERROR] file not found: %s\n", path);

        return -1;
    }

    char file_buffer[BUFFER_SIZE];
    int bytes_read;

    Packet pkt;
    int seq = 0;
    // read bytes --> BUFFER_SIZE per steps
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
        // addition in sequence number
        pkt.seq_number = seq++;
        // lenght of data in bytes
        pkt.payload_length = bytes_read;
        // copy data into payload
        memcpy(pkt.payload, file_buffer, bytes_read);
        
        // TO DO Later
        // pkt.check_sum
        // pkt.window

        char send_buf[sizeof(pkt)];
        int size = serialize_packet(&pkt, send_buf);

        sendto(sockfd, send_buf, size, 0, (struct sockaddr *)dest_addr, dest_len);
        printf("Packet Seq %d was sent by server, ack %d", pkt.seq_number, pkt.ack_number);

        // To DO Implement Stop and Wait

    }
    fclose(fp);
    printf("[SUCCESS] File sent to client\n");
}



