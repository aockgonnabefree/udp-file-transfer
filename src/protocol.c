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

int perform_handshake(int sockfd, struct sockaddr_in *server_addr, char *filename){
    printf("Start 3 way handshake\n");

    // 1: Client Send SYN
    Packet syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_number = 0;

    if (sendto(sockfd, &syn_packet, sizeof(syn_packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("[ERROR] sending SYN packet");
        return -1;
    }

    printf("SYN packet sent. Sequence number: %d\n", syn_packet.seq_number);

    // Step 2: Client รอรับ SYN-ACK
    Packet syn_ack_packet;
    if (recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, NULL, NULL) < 0) {
        perror("[ERROR] receiving SYN-ACK packet");
        return -1;
    }

    // ตรวจสอบว่าแพ็กเก็ตที่ได้รับเป็น SYN-ACK หรือไม่
    if ((syn_ack_packet.flags & FLAG_SYN) && (syn_ack_packet.flags & FLAG_ACK)) {
        printf("SYN-ACK packet received. Acknowledgment number: %d\n", syn_ack_packet.ack_number);

        // **Step 3: Client ส่ง ACK พร้อมข้อมูล (Piggybacking)**
        Packet ack_packet_with_data;
        memset(&ack_packet_with_data, 0, sizeof(ack_packet_with_data));
        
        // ตั้งค่า flag เป็น ACK และ DATA
        ack_packet_with_data.flags = FLAG_ACK | FLAG_DATA;
        ack_packet_with_data.seq_number = syn_ack_packet.ack_number;
        ack_packet_with_data.ack_number = syn_ack_packet.seq_number + 1;
        
        ack_packet_with_data.payload_length = strlen(filename);
        memcpy(ack_packet_with_data.payload, filename, ack_packet_with_data.payload_length);
        
        // ส่งแพ็กเก็ต ACK ที่มีข้อมูล
        if (sendto(sockfd, &ack_packet_with_data, sizeof(ack_packet_with_data), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
            perror("[ERROR] sending ACK with data packet");
            return -1;
        }
        printf("ACK packet sent with file data. Handshake successful!\n");
        return 0;
    } else {
        printf("Received unexpected packet type. Handshake failed.\n");
        return -1;
    }


}


