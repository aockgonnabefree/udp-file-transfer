#include <stdio.h>

#include "packet.h"

#define MAX_PAYLOAD_SIZE 1024

#define FLAG_DATA 0b00000000 
#define FLAG_ACK  0b00000001 
#define FLAG_FIN  0b00000010 
#define FLAG_SYN  0b00000100

typedef struct{
    /*
    
    Define My FTP packets look like

    seq_number     (32-bit) : บอก sequence number ของ packet ที่กำลังส่ง
    ack_number     (32-bit) : บอก acknowledgment number ของ packet ที่ผู้รับต้องการรับต่อไป
    payload_length (16-bit) : บอกขนาดของ payload ป้องกันกรณี payload ไม่เต็มขนาด
    check_sum      (16-bit) : เอาไว้ทำ checksum
    window         (16-bit) : เอาไว้บอกว่าผู้รับสามารถรับได้อีกกี่ไบต์
    flags          (8-bit)  : บอกว่า payload นี้เป็นข้อมูลประเภทไหน 
        DATA    (0000 0000) -> payload เป็นข้อมูล
        ACK     (0000 0001) -> payload เป็น Ackknowledgement
        FIN     (0000 0010) -> payload เป็น Packet สุดท้าย

    payload         (payload size - bit) : เก็บข้อมูลที่กำลังส่ง
    
    */
    u_int32_t seq_number;
    u_int32_t ack_number;
    
    u_int16_t payload_length;
    u_int16_t window;
    u_int16_t check_sum;

    u_int8_t flags;

    char payload[MAX_PAYLOAD_SIZE];
} Packet;

int serialize(Packet *pkt, unsigned char *buffer) {
    /*
    Function to Serialize Struct --> Array of Bytes
    */
   return 1;
}

int deserialize(unsigned char *buffer, int size, Packet *pkt) {
    /*
    Function to Deserialize Array of Bytes --> Struct
    */
   return 1;
}

u_int16_t calculate_checksum(Packet *pkt) {
    // 1. ตั้งค่าฟิลด์ checksum ใน packet ให้เป็น 0 ชั่วคราว
    pkt->check_sum = 0;

    u_int32_t sum = 0;

    // 2. คำนวณ checksum สำหรับส่วน header
    sum += (pkt->seq_number >> 16) & 0xFFFF;
    sum += pkt->seq_number & 0xFFFF;

    sum += (pkt->ack_number >> 16) & 0xFFFF;
    sum += pkt->ack_number & 0xFFFF;
    
    sum += pkt->payload_length;
    sum += pkt->window;
    sum += pkt->flags;

    // 3. คำนวณ checksum สำหรับ payload
    u_int16_t *payload_ptr = (u_int16_t *)pkt->payload;
    size_t payload_size = pkt->payload_length;

    while (payload_size > 1) {
        sum += *payload_ptr++;
        payload_size -= 2;
    }
    // ถ้า payload มีขนาดเป็นเลขคี่ ให้บวกไบต์สุดท้าย
    if (payload_size > 0) {
        sum += *(u_int8_t *)payload_ptr;
    }
    
    // 4. รวมค่าส่วนเกิน (carry) กลับในผลรวม
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // 5. กลับบิต (one's complement) ของผลรวมสุดท้าย
    return (u_int16_t)~sum;
}