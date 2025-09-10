#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define MAX_PAYLOAD_SIZE 1024
#define FLAG_DATA 0b00000001 // 1: Packet นี้มีข้อมูลไฟล์
#define FLAG_ACK  0b00000010 // 2: Packet นี้เป็น Acknowledgement
#define FLAG_SYN  0b00000100 // 4: Packet นี้ใช้สำหรับ Synchronize (เริ่มต้นการเชื่อมต่อ)
#define FLAG_FIN  0b00001000 // 8: Packet นี้ใช้สำหรับ Finish (สิ้นสุดการส่งไฟล์)


// Packet structure
typedef struct {
    /*
    
    Define My FTP packets look like

    seq_number     (32-bit) : บอก sequence number ของ packet ที่กำลังส่ง
    ack_number     (32-bit) : บอก acknowledgment number ของ packet ที่ผู้รับต้องการรับต่อไป
    payload_length (16-bit) : บอกขนาดของ payload ป้องกันกรณี payload ไม่เต็มขนาด
    check_sum      (16-bit) : เอาไว้ทำ checksum
    window         (16-bit) : เอาไว้บอกว่าผู้รับสามารถรับได้อีกกี่ไบต์
    flags          (8-bit)  : บอกว่า payload นี้เป็นข้อมูลประเภทไหน 
    payload        (payload size - bit) : เก็บข้อมูลที่กำลังส่ง
    
    */
    uint32_t seq_number;
    uint32_t ack_number;
    uint16_t payload_length;
    uint16_t window;
    uint16_t check_sum;
    uint8_t  flags;
    char payload[MAX_PAYLOAD_SIZE];
} Packet;

// Serialize struct → buffer
int serialize_packet(const Packet *pkt, char *buffer);

// Deserialize buffer → struct
int deserialize_packet(const char *buffer, int size, Packet *pkt);

// Checksum utility
uint16_t calculate_checksum(Packet *pkt);

#endif
