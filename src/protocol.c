#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "packet.h"

#define IP "127.0.0.1"
#define PORT 8080

#define BUFFER_SIZE 1024

// int send_file(int sockfd, const char *filename, struct sockaddr_in *dest_addr, socklen_t dest_len) {
//     char path[256];
//     snprintf(path, sizeof(path), "../example/%s", filename);
//     FILE *fp = fopen(path, "r");
    
//     // Handle in case File not found
//     if (fp == NULL) {
//         printf("[ERROR] file not found: %s\n", path);

//         return -1;
//     }

//     char file_buffer[BUFFER_SIZE];
//     int bytes_read;

//     Packet pkt;
//     int seq = 0;
//     // read bytes --> BUFFER_SIZE per steps
//     while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
//         // addition in sequence number
//         pkt.seq_number = seq++;
//         // lenght of data in bytes
//         pkt.payload_length = bytes_read;
//         // copy data into payload
//         memcpy(pkt.payload, file_buffer, bytes_read);
        
//         // TO DO Later
//         // pkt.check_sum
//         // pkt.window

//         char send_buf[sizeof(pkt)];
//         int size = serialize_packet(&pkt, send_buf);

//         sendto(sockfd, send_buf, size, 0, (struct sockaddr *)dest_addr, dest_len);
//         printf("Packet Seq %d was sent by server, ack %d", pkt.seq_number, pkt.ack_number);

//         // To DO Implement Stop and Wait

//     }
//     fclose(fp);
//     printf("[SUCCESS] File sent to client\n");
// }

int server_handle_syn(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len) {
    // Ready to handshake
    Packet recv_pkt;

    // Step 1: รับ SYN Packet
    recvfrom(sockfd, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)client_addr, &client_len);
    
    if (recv_pkt.flags != FLAG_SYN) 
        return -1;

    // Step 2: ส่ง SYN-ACK 
    printf("[SERVER] Received SYN (seq=%d)\n", recv_pkt.seq_number);
    Packet syn_ack_pkt;
    syn_ack_pkt.seq_number = 1000; // กำหนด Sequence Number ของฝั่ง server เริ่มที่ 1000
    syn_ack_pkt.ack_number = recv_pkt.seq_number + 1;
    syn_ack_pkt.flags = FLAG_ACK | FLAG_SYN;

    sendto(sockfd, &syn_ack_pkt, sizeof(syn_ack_pkt), 0, (struct sockaddr *)client_addr, client_len);
    printf("[SERVER] SYN-ACK packet sent. seq#: %d, ack#: %d\n", syn_ack_pkt.seq_number, syn_ack_pkt.ack_number); 

    // Step 3: รับ ACK-DATA
    recvfrom(sockfd, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)client_addr, &client_len);
    if (recv_pkt.flags & (FLAG_ACK | FLAG_DATA)) { 
        printf("[SERVER] Received ACK-DATA (ack=%d)\n", recv_pkt.ack_number);
        printf("[SERVER] Handshake complete!\n");

        printf("[SERVER] Client requested file %s\n", recv_pkt.payload);
        return 0;
    }
    return -1;
}

int client_perform_handshake(int sockfd, struct sockaddr_in *server_addr, socklen_t server_len, char *filename){
    printf("Request File to Server. Starting 3 way handshake\n");

    // 1: Client Send SYN
    Packet syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_number = 0; // กำหนด Sequence Number ฝั่ง Client เริ่มที่ 0

    if (sendto(sockfd, &syn_packet, sizeof(syn_packet), 0, (struct sockaddr *)server_addr, server_len) < 0) {
        printf("[CLIENT][ERROR] sending SYN packet");
        return -1;
    }

    printf("[SERVER] SYN packet sent. seq#: %d, ack#: %d\n", syn_packet.seq_number, syn_packet.ack_number);

    // Step 2: Client รอรับ SYN-ACK
    Packet syn_ack_packet;
    if (recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, (struct sockaddr *)server_addr, &server_len) < 0) {
        printf("[CLIENT][ERROR] receiving SYN-ACK packet");
        return -1;
    }

    // ตรวจสอบว่าแพ็กเก็ตที่ได้รับเป็น SYN-ACK หรือไม่
    if ((syn_ack_packet.flags & (FLAG_ACK | FLAG_SYN)) != (FLAG_ACK | FLAG_SYN)) {
        printf("[CLIENT][ERROR] Received unexpected packet type. Handshake failed.\n");
        return -1;
    }
    // Step 3: Client ส่ง ACK-DATA
    printf("[CLIENT] Received SYN-ACK. ack#: %d\n", syn_ack_packet.ack_number);
    Packet ack_packet_with_data;
    memset(&ack_packet_with_data, 0, sizeof(ack_packet_with_data));
    ack_packet_with_data.flags = FLAG_ACK | FLAG_DATA;
    ack_packet_with_data.seq_number = syn_ack_packet.ack_number;
    ack_packet_with_data.ack_number = syn_ack_packet.seq_number + 1;
    
    // Data -> filename ที่ต้องการดาวน์โหลด
    ack_packet_with_data.payload_length = strlen(filename);
    memcpy(ack_packet_with_data.payload, filename, ack_packet_with_data.payload_length);
    
    if (sendto(sockfd, &ack_packet_with_data, sizeof(ack_packet_with_data), 0, (struct sockaddr *)server_addr, server_len) < 0) {
        printf("[ERROR] sending ACK with data packet");
        return -1;
    }

    printf("[CLIENT] ACK-DATA packet sent., seq#: %d, ack#: %d. Handshake successful!\n", ack_packet_with_data.seq_number, ack_packet_with_data.ack_number);
    return 0;
}

