# เอกสารการออกแบบ: UDP File Transfer with Reliability

## 1. ภาพรวม (Overview)

เอกสารนี้อธิบายการออกแบบและการพัฒนาโปรโตคอลสำหรับส่งไฟล์ที่มีความน่าเชื่อถือ โดยสร้างบนพื้นฐานของ UDP ระบบนี้ใช้กลไกความน่าเชื่อถือที่กำหนดเองรวมถึงการตรวจจับข้อผิดพลาด การยืนยันการรับข้อมูล และการส่งซ้ำ เพื่อให้แน่ใจว่าข้อมูลถูกส่งได้อย่างน่าเชื่อถือบน transport layer ที่ไม่น่าเชื่อถือ

---

## 2. การออกแบบโปรโตคอล (Protocol Design)

### 2.1 รูปแบบของ Packet (Packet Format)

โครงสร้าง packet ที่กำหนดเองประกอบด้วย header และ payload:

```
┌──────────────────────────────────────────────────┐
│         โครงสร้าง Packet (1039+ bytes)            │
├──────────────────────────────────────────────────┤
│  seq_number      (32-bit / 4 bytes)              │  หมายเลขลำดับ
│  ack_number      (32-bit / 4 bytes)              │  หมายเลขยืนยันการรับ
│  payload_length  (16-bit / 2 bytes)              │  ขนาด payload จริง
│  window          (16-bit / 2 bytes)              │  หน้าต่าง flow control
│  check_sum       (16-bit / 2 bytes)              │  ตรวจจับข้อผิดพลาด
│  flags           (8-bit  / 1 byte)               │  ประเภท packet
├──────────────────────────────────────────────────┤
│  payload         (1024 bytes)                    │  ข้อมูลไฟล์
└──────────────────────────────────────────────────┘
ขนาด Header รวม: 15 bytes
ขนาด Packet รวม: 1039 bytes (สูงสุด)
```

#### คำอธิบายแต่ละฟิลด์:

- **seq_number** (32-bit): บอก byte offset ของ byte แรกใน payload
- **ack_number** (32-bit): บอก byte ถัดไปที่คาดหวังจากผู้ส่ง
- **payload_length** (16-bit): จำนวน bytes จริงใน payload (0-1024)
- **window** (16-bit): พื้นที่ buffer ว่างของผู้รับ (ยังไม่ได้ใช้งาน)
- **check_sum** (16-bit): Internet checksum สำหรับตรวจจับข้อผิดพลาด
- **flags** (8-bit): bit flags บอกประเภทของ packet
  - `FLAG_DATA` (0x01): Packet มีข้อมูลไฟล์
  - `FLAG_ACK`  (0x02): Packet ยืนยันการรับ
  - `FLAG_SYN`  (0x04): Synchronization (เริ่มต้นการเชื่อมต่อ)
  - `FLAG_FIN`  (0x08): Finish (สิ้นสุดการเชื่อมต่อ)

### 2.2 การสร้างการเชื่อมต่อ (3-Way Handshake)

โปรโตคอลใช้ 3-way handshake แบบ optimized คล้ายกับ TCP:

```
Client                                    Server
  |                                         |
  |  SYN (seq=0)                            |
  |──────────────────────────────────────>  │
  |                                         │ เริ่มต้น connection
  |                                         │ seq_num = 1000
  |          SYN-ACK (seq=1000, ack=1)      │
  |<──────────────────────────────────────  │
  │                                         │
  │  ACK-DATA (seq=1, ack=1001, data=filename)
  |──────────────────────────────────────>  │
  │                                         │
  │           สร้างการเชื่อมต่อสำเร็จ             │
  │                                         │
```

**การปรับปรุง**: ACK ตัวสุดท้ายจาก client รวมชื่อไฟล์ที่ต้องการเป็น payload ช่วยลดการส่งข้อมูลไปมาอีกหนึ่งรอบ

**การใช้งาน**: `protocol.c`
- Client: `client_perform_handshake()` (บรรทัด 291-358)
- Server: `server_handle_syn()` (บรรทัด 227-289)

### 2.3 การส่งข้อมูล (Data Transfer)

หลังจากสร้างการเชื่อมต่อแล้ว server จะส่งไฟล์โดยใช้กลไก Stop-and-Wait ARQ:

```
Server                                    Client
  |                                         |
  |  DATA (seq=1001, len=1024)              |
  |──────────────────────────────────────>  │
  |                                         │ ตรวจสอบ checksum
  |                                         │ เขียนลงไฟล์
  |          ACK (ack=2025)                 │
  |<──────────────────────────────────────  │
  │                                         │
  │  DATA (seq=2025, len=1024)              │
  |──────────────────────────────────────>  │
  │          ...                            │
```

**การใช้งาน**: `protocol.c`
- Server: `send_file()` (บรรทัด 14-142)
- Client: `receive_file()` (บรรทัด 144-225)

### 2.4 การปิดการเชื่อมต่อ (Connection Termination)

หลังจากส่งข้อมูลทั้งหมดแล้ว server จะเริ่มการปิดการเชื่อมต่อ:

```
Server                                    Client
  |                                         |
  |  FIN (seq=N+1)                          |
  |──────────────────────────────────────>  │
  |                                         │
  |          FIN-ACK (ack=N+2)              │
  |<──────────────────────────────────────  │
  │                                         │
  │           ปิดการเชื่อมต่อแล้ว                │
```

---

## 3. กลไกความน่าเชื่อถือ (Reliability Mechanisms)

### 3.1 Stop-and-Wait ARQ

**เหตุผลในการเลือกใช้**: Stop-and-Wait ARQ ถูกเลือกเพราะมีความเรียบง่ายและง่ายต่อการพัฒนา พร้อมทั้งรับประกันการส่งที่น่าเชื่อถือ

**วิธีการทำงาน**:
1. ผู้ส่งส่ง packet หนึ่งตัวและเริ่ม timer
2. ผู้ส่งรอ ACK ก่อนส่ง packet ตัวถัดไป
3. ถ้าได้รับ ACK: ส่ง packet ถัดไป
4. ถ้า timeout เกิดขึ้น: ส่ง packet เดิมซ้ำ

**ข้อดี**:
- ง่ายต่อการพัฒนาและ debug
- ต้องใช้ buffer น้อย
- รับประกันการส่งตามลำดับ

**ข้อเสีย**:
- Throughput ต่ำ (จำกัดที่ 1 packet ต่อ RTT)
- ใช้ bandwidth ไม่มีประสิทธิภาพ โดยเฉพาะในเครือข่ายที่มี latency สูง

**ตำแหน่งในโค้ด**: `protocol.c:56-100`

```c
while (!ack_receive) {
    // ส่ง packet
    sendto(sockfd, &temp_pkt, sizeof(temp_pkt), ...);

    // รอ ACK พร้อม timeout
    int n = recvfrom(sockfd, &ack_from_client_pkt, ...);

    if (n < 0) {
        // Timeout - ส่งใหม่
        printf("[SERVER] Timeout, resending seq=%d\n", ...);
        continue;
    }

    // ตรวจสอบ ACK
    if (ack_from_client_pkt.ack_number == expected) {
        ack_receive = 1;  // ไปยัง packet ถัดไป
    }
}
```

### 3.2 การตั้งค่า Timeout

- **ค่า Timeout**: 1 วินาที (ค่าคงที่)
- **การใช้งาน**: `setsockopt()` ด้วย `SO_RCVTIMEO`
- **ตำแหน่ง**: `protocol.c:30-31`, `protocol.c:313-314`

```c
struct timeval time_interval = {1, 0};  // 1 วินาที
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time_interval, sizeof(time_interval));
```

### 3.3 Checksum สำหรับตรวจจับข้อผิดพลาด

**อัลกอริทึม**: Internet Checksum (16-bit one's complement)

**การใช้งาน**: `packet.c:7-44`

```c
uint16_t calculate_checksum(Packet *pkt) {
    pkt->check_sum = 0;
    uint32_t sum = 0;

    // รวมฟิลด์ใน header (seq, ack, length, window, flags)
    sum += (pkt->seq_number >> 16) & 0xFFFF;
    sum += pkt->seq_number & 0xFFFF;
    // ... ฟิลด์อื่นๆ

    // รวม payload (16-bit words)
    uint16_t *payload_ptr = (uint16_t *)pkt->payload;
    size_t payload_size = pkt->payload_length;
    while (payload_size > 1) {
        sum += *payload_ptr++;
        payload_size -= 2;
    }

    // จัดการ byte คี่
    if (payload_size > 0) {
        sum += *(uint8_t *)payload_ptr;
    }

    // พับ carry และ return one's complement
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}
```

**การตรวจสอบ**: `packet.c:46-61`
- ผู้รับคำนวณ checksum ใหม่จาก packet ที่ได้รับ
- เปรียบเทียบกับฟิลด์ checksum
- Packet ที่เสียหายจะถูกทิ้ง (ผู้ส่งจะ timeout และส่งซ้ำ)

### 3.4 การจัดการ ACK (ACK Handling)

**Cumulative ACKs**: หมายเลข ACK บอก byte ถัดไปที่คาดหวัง:
- ถ้า server ส่ง seq=1001 พร้อม 1024 bytes
- Client ตอบกลับด้วย ack=2025 (1001 + 1024)

**การจัดการ Duplicate ACK** (`protocol.c:200-211`):
- Client อาจได้รับ packet ซ้ำหรือไม่เรียงลำดับ
- Client ส่ง ACK พร้อม expected_seq เสมอ
- Client เขียนลงไฟล์เฉพาะเมื่อ seq ตรงกับ expected_seq
- จัดการกับการเรียงลำดับใหม่และการส่งซ้ำที่ซ้ำซ้อน

---

## 4. การจำลองข้อผิดพลาด (Error Simulation)

เพื่อทดสอบกลไกความน่าเชื่อถือ ระบบมีการจำลองข้อผิดพลาดในตัว

### 4.1 การจำลอง Packet Loss

**การใช้งาน**: `utils/udtSimulate.c:23-32`

```c
int packetLost(float lossProbability) {
    float random_val = (float)rand() / (float)RAND_MAX;

    if (random_val < lossProbability) {
        printf("\n[SIMULATOR] >>> Packet lost <<<\n\n");
        return 1;  // Packet สูญหาย
    }
    return 0;  // Packet ไม่สูญหาย
}
```

**การใช้** (`protocol.c:66-72`):
```c
if (!packetLost(0.01)) {  // อัตราสูญหาย 1%
    sendto(sockfd, &temp_pkt, ...);
} else {
    printf("[SIMULATOR] Packet (seq=%d) was lost.\n", ...);
}
```

**การปรับแต่ง**: เปลี่ยนค่า probability parameter (ปัจจุบัน 0.01 = 1%)

### 4.2 การจำลอง Packet Corruption

**การใช้งาน**: `utils/udtSimulate.c:7-21`

```c
int packetCorrupted(Packet *pkt, float corruptionProbability) {
    float random_val = (float)rand() / (float)RAND_MAX;

    if (random_val < corruptionProbability) {
        if (pkt->payload_length > 0) {
            printf("\n[SIMULATOR] >>> Corrupting packet <<<\n\n");
            pkt->payload[0] = ~pkt->payload[0];  // กลับบิตทั้งหมดของ byte แรก
        }
        return 1;
    }
    return 0;
}
```

**การใช้** (`protocol.c:64`):
```c
Packet temp_pkt = data_pkt;
packetCorrupted(&temp_pkt, 0.01);  // อัตราความเสียหาย 1%
```

**การปรับแต่ง**: ปรับค่า probability parameter (ปัจจุบัน 0.01 = 1%)

### 4.3 การทดสอบการจัดการข้อผิดพลาด

เมื่อเกิดข้อผิดพลาด ระบบจะตอบสนองดังนี้:

| ประเภทข้อผิดพลาด | การตรวจจับ | การกู้คืน |
|------------|-----------|----------|
| Packet Loss | Timeout ที่ผู้ส่ง | ส่งซ้ำ (Retransmission) |
| Packet Corruption | การตรวจสอบ Checksum ล้มเหลว | ทิ้ง Packet, timeout จะทำให้ส่งซ้ำ |
| Duplicate Packet | seq != expected_seq ที่ผู้รับ | ส่ง ACK แต่ไม่เขียนข้อมูล |
| Out-of-order Packet | seq != expected_seq ที่ผู้รับ | ส่ง ACK แต่ไม่เขียนข้อมูล |

---

## 5. รายละเอียดการพัฒนา (Implementation Details)

### 5.1 การจัดการไฟล์ (File Handling)

**Server** (`protocol.c:16-23`):
- อ่านไฟล์จาก `./example/<filename>`
- อ่านทีละชิ้น 1024 bytes (BUFFER_SIZE)
- ใช้โหมด binary (`"rb"`) สำหรับไฟล์ทุกประเภท

**Client** (`protocol.c:146-155`):
- เขียนไฟล์ไปที่ `./client_download/<filename>`
- สร้างไฟล์ในโหมด binary (`"wb"`)
- เขียนชิ้นข้อมูลที่ได้รับตามลำดับ

### 5.2 การจัดการ Sequence Number

**ค่าเริ่มต้น**:
- Client: seq_num = 0
- Server: seq_num = 1000

**Sequence Numbers แบบ Byte-oriented**:
- Sequence numbers แทน byte offsets
- หลังส่ง N bytes: seq_num += N
- ACK numbers บอก byte ถัดไปที่คาดหวัง

**ตัวอย่าง**:
```
Server ส่ง: seq=1001, len=1024
Client คาดหวัง: ack=2025 (1001+1024)
Server ส่ง: seq=2025, len=1024
Client คาดหวัง: ack=3049 (2025+1024)
```

---

## 6. ข้อจำกัดและการปรับปรุงในอนาคต (Limitations and Future Improvements)

### 6.1 ข้อจำกัดปัจจุบัน

1. **Throughput ต่ำ**
   - Stop-and-Wait จำกัดที่ 1 packet ต่อ RTT
   - สำหรับ packet 1 KB กับ RTT 50ms: ทฤษฎีสูงสุด ~20 KB/s
   - Throughput จริงต่ำกว่าเนื่องจาก overhead และการส่งซ้ำ

2. **Timeout แบบคงที่**
   - Timeout 1 วินาทีไม่ปรับตัว
   - ยาวเกินไปสำหรับเครือข่าย low-latency (เสียเวลา)
   - สั้นเกินไปสำหรับเครือข่าย high-latency (ส่งซ้ำโดยไม่จำเป็น)

3. **ไม่มี Congestion Control**
   - ไม่ตรวจจับหรือตอบสนองต่อความแออัดของเครือข่าย
   - อาจทำให้เกิด congestion collapse ในเครือข่ายที่มีการใช้งานหนัก

### 6.2 การปรับปรุงที่แนะนำ

1. **Adaptive Timeout** (การประมาณค่า RTT แบบ TCP)
   ```c
   EstimatedRTT = (1-α) × EstimatedRTT + α × SampleRTT
   Timeout = EstimatedRTT + 4 × DevRTT
   ```
2. **Pipelining**
   - ส่งหลาย packets ก่อนรอ ACKs
   - ติดตาม in-flight packets
   - ปรับปรุง throughput อย่างมากใน high-latency links
3. **Selective Repeat ARQ**
   - แทนที่ Stop-and-Wait ด้วย Selective Repeat
   - มี send/receive buffers
   - รับ packet ที่ไม่เรียงลำดับได้
   - คาดว่าจะปรับปรุง throughput ได้ดียิ่งขึ้น

4. **Congestion Control**
   - พัฒนา slow start และ congestion avoidance
   - AIMD (Additive Increase, Multiplicative Decrease)
   - Fast retransmit/fast recovery
---

## 7. สรุป (Conclusion)

การพัฒนานี้แสดงให้เห็นหลักการพื้นฐานของการส่งข้อมูลที่น่าเชื่อถือผ่านช่องทางที่ไม่น่าเชื่อถือ แม้ว่า Stop-and-Wait ARQ จะเรียบง่ายและถูกต้อง แต่ก็เสียสละประสิทธิภาพ ความสามารถในการจำลองข้อผิดพลาดช่วยให้ทดสอบกลไกความน่าเชื่อถือได้อย่างละเอียด

การออกแบบประสบความสำเร็จในการบรรลุ:
- ✅ การส่งไฟล์ที่น่าเชื่อถือผ่าน UDP
- ✅ การตรวจจับข้อผิดพลาดผ่าน checksums
- ✅ การส่งซ้ำอัตโนมัติเมื่อสูญหาย/เสียหาย
- ✅ การรับประกันการส่งตามลำดับ
- ✅ ความหมายแบบ connection-oriented

สำหรับการใช้งานจริง การพัฒนาการปรับปรุงที่แนะนำ (โดยเฉพาะ Selective Repeat, adaptive timeout และ congestion control) จะมีความจำเป็นเพื่อให้ได้ประสิทธิภาพที่ยอมรับได้

---

## อ้างอิง (References)

- Computer Networking: A Top-Down Approach (Kurose & Ross)