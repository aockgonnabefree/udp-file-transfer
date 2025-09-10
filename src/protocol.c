#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "packet.h"
#include "protocol.h"

#define IP "127.0.0.1"
#define PORT 8080

#define BUFFER_SIZE 1024

int send_file(int sockfd, struct sockaddr_in *dest_addr, socklen_t dest_len, connection_t *conn) {
    // Step 1: เปิดไฟล์เพื่อเตรียมอ่าน
    char path[256];
    snprintf(path, sizeof(path), "./example/%s", conn->filename);
    FILE *fp = fopen(path, "r");
    // Handle case File not found
    if (fp == NULL) {
        printf("[ERROR] file not found: %s\n", path);
        return -1;
    }

    char file_buffer[BUFFER_SIZE];
    int bytes_read;

    // read data in bytes --> BUFFER_SIZE per steps
    
    // Step 1.1: เตรียม socket timeout timing เพื่อ retransmit
    struct timeval time_interval = {1, 0}; // 1 วินาที ในการรอ ACK ก่อนส่งใหม่
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time_interval, sizeof(time_interval)); // function สำหรับ setting time out

    // Step 1.3: เตรียมตัวแปร
    // data_pkt: สำหรับ pkt ที่ส่งข้อมูล 
    // ack_from_client_pkt: สำหรับ pkt ที่ client ส่งมาเพื่อ ACK
    // fin_pkt: สำหรับ pkt เพื่อส่งบอก client ว่าเสร็จสิ้นการส่งไฟล์
    Packet data_pkt, ack_from_client_pkt, fin_pkt;
    int seq = conn->seq_num + 1; // next sequence number from handshake
    // Step 2: transfer file phase
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
        // Step 2.1: กำหนดข้อมูลของ DATA pkt
        conn->seq_num = seq; // กำหนด seq number
        data_pkt.seq_number = conn->seq_num;
        data_pkt.ack_number = conn->ack_num;
        data_pkt.flags = FLAG_DATA; // Packet เป็นการส่งข้อมูล
        
        // copy data into payload, tell bytes length
        memcpy(data_pkt.payload, file_buffer, bytes_read);
        data_pkt.payload_length = bytes_read;
        
        // Must TO DO
        // serialize
        // checksum
        // char send_buf[sizeof(data_pkt)];
        // int size = serialize_packet(&data_pkt, send_buf);
        // sendto(sockfd, send_buf, size, 0, (struct sockaddr *)dest_addr, dest_len);

        // Step 2.2: stop and wait implementation
        int ack_receive = 0;
        while (!ack_receive) {
            // send DATA pkt
            sendto(sockfd, &data_pkt, sizeof(data_pkt), 0, (struct sockaddr *)dest_addr, dest_len);
            printf("[SERVER] DATA packet sent. seq#: %d, ack#: %d\n", data_pkt.seq_number, data_pkt.ack_number);
            
            int n = recvfrom(sockfd, &ack_from_client_pkt, sizeof(ack_from_client_pkt), 0, (struct sockaddr *)dest_addr, &dest_len);

            // Timeout
            if (n < 0) {
                printf("[SERVER] Timeout, resending seq=%d\n", data_pkt.seq_number);
                continue;
            }

            // Check if it correct ACK packet
            if (ack_from_client_pkt.ack_number == data_pkt.seq_number + data_pkt.payload_length) {
                ack_receive = 1; // ACK ถูกต้อง -> ส่ง packet ต่อไป        
                printf("[SERVER] ACK pkt recieved (ack=%d)\n", ack_from_client_pkt.ack_number);
                seq += bytes_read;
            } else {
                // Duplicate/ผิด ACK -> resend
                printf("[SERVER] Wrong ACK received: %d, resending seq=%d\n", ack_from_client_pkt.ack_number, data_pkt.seq_number);
            }
        }
    }

    // Step 3: send FIN packet into Client to tell that it's finished of sending file
    fin_pkt.seq_number = seq+1;
    fin_pkt.payload_length = 0;
    fin_pkt.flags = FLAG_FIN;

    // stop and wait for FIN-ACK pkt
    int fin_ack_receive = 0;
    while (!fin_ack_receive){
        sendto(sockfd, &fin_pkt, sizeof(fin_pkt), 0, (struct sockaddr *)dest_addr, dest_len);
        printf("[SERVER] FIN packet sent\n");

        int n = recvfrom(sockfd, &ack_from_client_pkt, sizeof(ack_from_client_pkt), 0, (struct sockaddr *)dest_addr, &dest_len);

        // timeout
        if (n < 0) {
            printf("[SERVER] Resending FIN\n");
        }

        if (n > 0 && ack_from_client_pkt.ack_number == fin_pkt.seq_number + 1 && !(ack_from_client_pkt.flags ^ (FLAG_FIN | FLAG_ACK))) {
            printf("[SERVER] FIN-ACK receive\n");
            fin_ack_receive = 1;
        } else {
            printf("[SERVER] Resending FIN\n");
        }
    }  

    fclose(fp);
    printf("[SUCCESS] File sent to client\n");
    return 0;
}

int receive_file(int server_sockfd, const char *filename, connection_t *conn) {
    // Step 1: สร้างไฟลเพื่อเตรียมเขียนข้อมูล
    char path[256];
    snprintf(path, sizeof(path), "./client_download/%s", filename);
    FILE *fp = fopen(path, "w"); // need to change to "wb" if data is sent in binary

    // Handle error case
    if (!fp) {
        printf("[CLIENT] Can not create file: %s\n", filename);
        close(server_sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    // Step 2: เตรียมตัวแปร
    // recv_pkt : pkt ที่รับมาจาก Server
    // ack_pkt : pkt ที่เอาไว้ส่ง ACK ไปยัง Server
    Packet recv_pkt, ack_pkt;
    int n;
    while (1) {
        n = recvfrom(server_sockfd, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&from_addr, &from_len);

        if (recv_pkt.flags & FLAG_FIN){
            printf("[CLIENT] FIN recieved\n");

            // send FIN-ACK to server
            ack_pkt.ack_number = recv_pkt.seq_number + 1;
            ack_pkt.flags = FLAG_FIN | FLAG_ACK;
            sendto(server_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *) &from_addr, from_len);
            printf("[CLIENT] FIN-ACK pkt sent\n");
            break;
        }

        printf("[CLIENT] DATA recieved (seq from server#: %d, ack#: %d)\n", recv_pkt.seq_number, recv_pkt.ack_number);

        if (n <= 0)
            break;

        // Check expected seq num -> if not correct sequence then request to restransmit

        // MUST TO DO (Checksum: if incorrect then request to restransmit)
        // Check Duplicate seq num
        // Check loss (Check sum)

        memcpy(buffer, recv_pkt.payload, recv_pkt.payload_length);
        fwrite(buffer, 1, sizeof(buffer), fp);

        // ACK to server that it's correct packat
        ack_pkt.seq_number = 1; // just simulate, but it need to implement by conn
        ack_pkt.ack_number = recv_pkt.seq_number + recv_pkt.payload_length; // next seq number need from server (start of next byte stream)
        sendto(server_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *) &from_addr, from_len);
        printf("[CLIENT] ACK pkt sent (sent ack==%d)\n", ack_pkt.ack_number);

        if (n < sizeof(buffer))
            break;
    }
    fclose(fp);
    printf("[SUCCESS] File received and saved as %s\n", filename);
}

int server_handle_syn(int sockfd, struct sockaddr_in *client_addr, socklen_t client_len, connection_t *conn) {
    // Ready to handshake
    Packet syn_pkt;
    // Step 1: รับ SYN Packet
    recvfrom(sockfd, &syn_pkt, sizeof(syn_pkt), 0, (struct sockaddr *)client_addr, &client_len);
    if (syn_pkt.flags != FLAG_SYN) 
        return -1;

    // สร้าง connection --> จำลอง TCP Connection-Oriented
    conn->seq_num = 1000;  // กำหนด Sequence Number ของฝั่ง server เริ่มที่ 1000
    conn->ack_num = syn_pkt.seq_number + 1; // คาดหวัง packet sequence ถัดไปจาก client
    printf("[SERVER] Received SYN (seq=%d)\n", syn_pkt.seq_number);

    // Step 2: ส่ง SYN-ACK 
    Packet syn_ack_pkt;
    syn_ack_pkt.seq_number = conn->seq_num;
    syn_ack_pkt.ack_number = conn->ack_num;
    syn_ack_pkt.flags = FLAG_SYN | FLAG_ACK;
    sendto(sockfd, &syn_ack_pkt, sizeof(syn_ack_pkt), 0, (struct sockaddr *)client_addr, client_len);
    printf("[SERVER] SYN-ACK packet sent. seq#: %d, ack#: %d\n", syn_ack_pkt.seq_number, syn_ack_pkt.ack_number); 

    // Step 3: รับ ACK-DATA
    Packet ack_data_pkt;
    recvfrom(sockfd, &ack_data_pkt, sizeof(ack_data_pkt), 0, (struct sockaddr *)client_addr, &client_len);
    if (! (ack_data_pkt.flags & (FLAG_ACK | FLAG_DATA))) 
        return -1;

    // update connection state
    conn->ack_num = ack_data_pkt.seq_number + 1; // คาดหวัง packet sequence ถัดไปจาก client
    // เก็บชื่อไฟล์ไว้ใน connection state
    memcpy(conn->filename, ack_data_pkt.payload, ack_data_pkt.payload_length);
    conn->filename[ack_data_pkt.payload_length] = '\0';

    printf("[SERVER] Received ACK-DATA (ack=%d)\n", ack_data_pkt.ack_number);
    printf("[SERVER] Handshake complete!\n");
    printf("[SERVER] Client requested file %s\n", ack_data_pkt.payload);
    return 0;
}

int client_perform_handshake(int sockfd, struct sockaddr_in *server_addr, socklen_t server_len, char *filename, connection_t *conn){
    /*
    Optimized 3-way handshake (Last ACK Client -> Server contains data in this case is Filename to request)
    */
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

    printf("[CLIENT] SYN packet sent. seq#: %d\n", syn_packet.seq_number);

    // Step 2: Client รอรับ SYN-ACK
    Packet syn_ack_packet;
    if (recvfrom(sockfd, &syn_ack_packet, sizeof(syn_ack_packet), 0, (struct sockaddr *)server_addr, &server_len) < 0) {
        printf("[CLIENT][ERROR] receiving SYN-ACK packet\n");
        return -1;
    }

    // ตรวจสอบว่าแพ็กเก็ตที่ได้รับเป็น SYN-ACK หรือไม่
    if ((syn_ack_packet.flags & (FLAG_ACK | FLAG_SYN)) != (FLAG_ACK | FLAG_SYN)) {
        printf("[CLIENT][ERROR] Received unexpected packet type. Handshake failed.\n");
        return -1;
    }
    // Step 3: Client ส่ง ACK-DATA (Optimized Fast Open)
    printf("[CLIENT] Received SYN-ACK (ack#: %d)\n", syn_ack_packet.ack_number);
    Packet ack_packet_with_data;
    memset(&ack_packet_with_data, 0, sizeof(ack_packet_with_data));
    ack_packet_with_data.flags = FLAG_ACK | FLAG_DATA;
    ack_packet_with_data.seq_number = syn_ack_packet.ack_number;
    ack_packet_with_data.ack_number = syn_ack_packet.seq_number + 1;
    
    // Data -> filename ที่ต้องการดาวน์โหลด (Optimized Part)
    ack_packet_with_data.payload_length = strlen(filename);
    memcpy(ack_packet_with_data.payload, filename, ack_packet_with_data.payload_length);
    
    if (sendto(sockfd, &ack_packet_with_data, sizeof(ack_packet_with_data), 0, (struct sockaddr *)server_addr, server_len) < 0) {
        printf("[ERROR] sending ACK with data packet\n");
        return -1;
    }

    printf("[CLIENT] ACK-DATA packet sent, seq#: %d, ack#: %d.\nHandshake successful!\n", ack_packet_with_data.seq_number, ack_packet_with_data.ack_number);
    return 0;
}

