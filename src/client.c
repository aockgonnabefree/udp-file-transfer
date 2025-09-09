#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <packet.c>

#define IP "127.0.0.1"
#define PORT 8080

int perform_handshake(int sockfd, struct sockaddr_in *server_addr){
    printf("Start 3 way handshake\n");

    // 1: Client Send SYN
    struct Packet syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_number = 0;

    if (sendto(sockfd, &syn_packet, sizeof(syn_packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("[ERROR] sending SYN packet");
        return -1;
    }

    printf("SYN packet sent. Sequence number: %d\n", syn_packet.seq_number);

    // Step 2: Client รอรับ SYN-ACK
    struct Packet syn_ack_packet;
    if (recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, NULL, NULL) < 0) {
        perror("[ERROR] receiving SYN-ACK packet");
        return -1;
    }

    // ตรวจสอบว่าแพ็กเก็ตที่ได้รับเป็น SYN-ACK หรือไม่
    if ((syn_ack_packet.flags & FLAG_SYN) && (syn_ack_packet.flags & FLAG_ACK)) {
        printf("SYN-ACK packet received. Acknowledgment number: %d\n", syn_ack_packet.ack_number);

        // **Step 3: Client ส่ง ACK พร้อมข้อมูล (Piggybacking)**
        struct Packet ack_packet_with_data;
        memset(&ack_packet_with_data, 0, sizeof(ack_packet_with_data));
        
        // ตั้งค่า flag เป็น ACK และ DATA
        ack_packet_with_data.flags = FLAG_ACK | FLAG_DATA;
        ack_packet_with_data.seq_number = syn_ack_packet.ack_number;
        ack_packet_with_data.ack_number = syn_ack_packet.seq_number + 1;
        
        // คัดลอกข้อมูลไฟล์ก้อนแรกใส่ใน payload
        char *filename = "example.c";
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("[ERROR] opening file for piggybacking");
            return -1;
        }
        size_t bytes_read = fread(ack_packet_with_data.payload, 1, MAX_PAYLOAD_SIZE, fp);
        fclose(fp);
        ack_packet_with_data.payload_length = bytes_read;
        
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