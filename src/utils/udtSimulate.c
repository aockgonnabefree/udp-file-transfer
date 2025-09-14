#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "packet.h"
#include "udtSimulate.h"

int packetCorrupted(Packet *pkt, float corruptionProbability) {
    // สร้างเลขสุ่มแบบ float ระหว่าง 0.0 ถึง 1.0
    float random_val = (float)rand() / (float)RAND_MAX;

    if (random_val < corruptionProbability) {
        // ทำให้ packet เสียหายถ้ามีข้อมูลใน payload
        if (pkt->payload_length > 0) {
            printf("\n[SIMULATOR] >>> Corrupting packet (seq=%d) <<<\n\n", pkt->seq_number);
            // สลับบิตทั้งหมดของ byte แรกใน payload เพื่อจำลอง corruption
            pkt->payload[0] = ~pkt->payload[0];
        }
        return 1; // Packet corrupted
    }
    return 0; // Packet not corrupted
}

int packetLost(float lossProbability) {
    // สร้างเลขสุ่มแบบ float ระหว่าง 0.0 ถึง 1.0
    float random_val = (float)rand() / (float)RAND_MAX;

    if (random_val < lossProbability) {
        printf("\n[SIMULATOR] >>> Packet lost <<<\n\n");
        return 1; // Packet is lost
    }
    return 0; // Packet is not lost
}