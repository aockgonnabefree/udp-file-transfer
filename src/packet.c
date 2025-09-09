#include <stdio.h>

#define MAX_PAYLOAD_SIZE 1024

#define FLAG_DATA 0x00 // 0000 0000
#define FLAG_ACK  0x01 // 0000 0001
#define FLAG_FIN  0x02 // 0000 0010
#define FLAG_SYN  0x03 // 0000 0011
struct Packet
{
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
        SYN     (0000 0011) -> payload เป็น Packet เริ่มต้น

    payload         (payload size - bit) : เก็บข้อมูลที่กำลังส่ง
    
    */
    u_int32_t seq_number;
    u_int32_t ack_number;
    
    u_int16_t payload_length;
    u_int16_t window;
    u_int16_t check_sum;

    u_int8_t flags;

    char payload[MAX_PAYLOAD_SIZE];
};
