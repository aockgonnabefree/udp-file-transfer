#include <stdio.h>

#include "packet.h"

#define MAX_PAYLOAD_SIZE 1024

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