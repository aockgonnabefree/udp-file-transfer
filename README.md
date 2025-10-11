# UDP File Transfer with Reliable Data Transfer

ระบบส่งไฟล์ที่มีความน่าเชื่อถือสร้างบนพื้นฐานของ UDP โดยใช้โปรโตคอลที่กำหนดเองพร้อมกลไกการตรวจจับข้อผิดพลาด การส่งซ้ำ และ flow control

## คุณสมบัติ (Features)

- **3-Way Handshake**: การสร้างการเชื่อมต่อคล้ายกับ TCP
- **Stop-and-Wait ARQ**: การส่งข้อมูลที่น่าเชื่อถือพร้อมการยืนยัน
- **Checksum Verification**: การตรวจจับข้อผิดพลาดสำหรับ packet ที่เสียหาย
- **Timeout & Retransmission**: การส่งซ้ำอัตโนมัติเมื่อ packet สูญหาย
- **Error Simulation**: การจำลอง packet loss และ corruption ในตัว (อัตรา 1% แต่ละอย่าง)
- **FIN Handshake**: การปิดการเชื่อมต่ออย่างสมบูรณ์

## โครงสร้างโปรเจค (Project Structure)

```
udp-file-transfer/
├── src/
│   ├── server.c              # การพัฒนาฝั่ง Server
│   ├── client.c              # การพัฒนาฝั่ง Client
│   ├── protocol.c            # Logic ของโปรโตคอล (handshake, send/receive)
│   ├── packet.c              # โครงสร้าง Packet และ checksum
│   ├── headers/
│   │   ├── protocol.h        # ประกาศฟังก์ชันโปรโตคอล
│   │   └── packet.h          # คำจำกัดความโครงสร้าง Packet
│   └── utils/
│       ├── udtSimulate.c     # เครื่องมือจำลองข้อผิดพลาด
│       └── udtSimulate.h
├── example/                  # ไฟล์ที่จะถูกส่ง (ฝั่ง server)
├── client_download/          # ไฟล์ที่ดาวน์โหลด (ฝั่ง client)
├── builds/                   # Binaries ที่ compile แล้ว
├── Makefile                  # การตั้งค่า Build
├── README.md                 # ไฟล์นี้
└── DESIGN.md                 # เอกสารการออกแบบ
```

## ความต้องการ (Requirements)

- GCC compiler
- ระบบปฏิบัติการ Linux/macOS/Unix-like
- การเชื่อมต่อเครือข่าย (สำหรับทดสอบข้ามเครื่อง)

## การ Compile

เพื่อ compile ทั้ง server และ client:

```bash
make
```

คำสั่งนี้จะ:
1. สร้าง directories ที่จำเป็น (`builds/objs`, `client_download/`)
2. ลบ builds ก่อนหน้า
3. Compile ไฟล์ source ทั้งหมด
4. สร้าง executables:
   - `builds/server.out`
   - `builds/client.out`

เพื่อลบไฟล์ build:

```bash
make clean
```

เพื่อลบไฟล์ที่ดาวน์โหลดในฝั่ง client:

```bash
make remove_client_file
```

## วิธีใช้งาน (Usage)

### Server

เริ่มต้น server บน port ที่ระบุ:

```bash
./builds/server.out <port>
```

**ตัวอย่าง:**
```bash
./builds/server.out 8080
```

Server จะ:
- Bind กับ network interfaces ทั้งหมด (`0.0.0.0`)
- Listen บน port ที่ระบุ
- รอ SYN packets ที่มาจาก clients
- ให้บริการไฟล์จาก directory `example/`

### Client

ขอไฟล์จาก server:

```bash
./builds/client.out <server_ip> <port> <filename>
```

**ตัวอย่าง:**

การทดสอบแบบ local:
```bash
./builds/client.out 127.0.0.1 8080 example.txt
```

Remote server:
```bash
./builds/client.out 10.31.22.230 8080 Chapter_1_v9.0.pptx
```

Client จะ:
- เชื่อมต่อกับ server IP และ port ที่ระบุ
- ทำ 3-way handshake
- ขอไฟล์ที่ระบุ
- ดาวน์โหลดและบันทึกไฟล์ไปที่ `client_download/`

## การทดสอบ (Testing)

### Test 1: การส่งไฟล์พื้นฐาน (Local)

1. **เตรียมไฟล์ทดสอบ:**
   ```bash
   # เพิ่มไฟล์ไปที่ directory example/
   cp /path/to/your/file example/testfile.txt
   ```

   Note: มีไฟล์การทดสอบให้ทั้งหมด 3 ไฟล์คือ
   - example.jpeg
   - example.txt
   - example.pptx

2. **Terminal 1 - เริ่ม server:**
   ```bash
   ./builds/server.out 8080
   ```

3. **Terminal 2 - รัน client:**
   ```bash
   ./builds/client.out 127.0.0.1 8080 testfile.txt
   ```

4. **ตรวจสอบ:**
   ```bash
   ls -lh client_download/
   diff example/testfile.txt client_download/testfile.txt
   ```

### Test 2: ไฟล์ประเภทต่างๆ

ทดสอบกับไฟล์หลายประเภทเพื่อให้แน่ใจว่าปลอดภัยกับ binary:

```bash
# ไฟล์ text
./builds/client.out 127.0.0.1 8080 example.txt

# ไฟล์ภาพ
./builds/client.out 127.0.0.1 8080 example.jpeg

# ไฟล์ PowerPoint
./builds/client.out 127.0.0.1 8080 Chapter_1_v9.0.pptx
```

ตรวจสอบความถูกต้อง:
```bash
md5sum example/example.jpeg client_download/example.jpeg
# หรือบน macOS:
md5 example/example.jpeg client_download/example.jpeg
```

### Test 3: การทดสอบเครือข่าย (เครื่องต่างกัน)

1. **บนเครื่อง Server:**
   ```bash
   # เริ่ม server
   ./builds/server.out 8080
   ```

2. **บนเครื่อง Client:**
   ```bash
   ./builds/client.out <server_ip> 8080 example.txt
   ```

### Test 4: การจำลองข้อผิดพลาด

ระบบจำลองอัตโนมัติ:
- **Packet Loss**: ความน่าจะเป็น 1% (ปรับแต่งได้ใน `protocol.c:66`)
- **Packet Corruption**: ความน่าจะเป็น 1% (ปรับแต่งได้ใน `protocol.c:64`)

สังเกต logs เพื่อดู:
- `[SIMULATOR] Packet lost` - Packet ถูกทิ้ง
- `[SIMULATOR] >>> Corrupting packet <<<` - Packet ถูกทำให้เสียหาย
- `[SERVER] Timeout, resending` - การส่งซ้ำถูกกระตุ้น
- `[CLIENT][ERROR] Corrupted packet received` - การตรวจสอบ Checksum ล้มเหลว

## ผลลัพธ์ที่คาดหวัง (Expected Output)

### Server Output:
```
[SERVER] Server started on port 8080
[SERVER] Waiting for SYN packet. (CTRL+C to exit)
[SERVER] Received SYN (seq=0)
[SERVER] SYN-ACK packet sent. seq#: 1000, ack#: 1
[SERVER] Received ACK-DATA (ack=1001)
[SERVER] Handshake complete!
[SERVER] Client requested file example.txt
[SERVER] DATA packet sent. seq#: 1001, ack#: 1
[SERVER] ACK pkt recieved (ack=1025)
...
[SERVER] FIN packet sent
[SERVER] FIN-ACK receive
[SUCCESS] File sent to client
```

### Client Output:
```
[CLIENT] Connecting to 127.0.0.1:8080
[CLIENT] Requesting file: example.txt
Request File to Server. Starting 3 way handshake
[CLIENT] SYN packet sent. seq#: 0, checksum: 0xXXXX
[CLIENT] Received SYN-ACK (ack#: 1)
[CLIENT] ACK-DATA packet sent, seq#: 1, ack#: 1001.
Handshake successful!
[CLIENT] DATA recieved (seq from server#: 1001, ack#: 1)
[CLIENT] Data written to file. Next expected seq: 1025
[CLIENT] ACK pkt sent (sent ack==1025)
...
[CLIENT] FIN recieved
[CLIENT] FIN-ACK pkt sent
[SUCCESS] File received and saved as example.txt
```

## การแก้ปัญหา (Troubleshooting)

### Port ถูกใช้งานอยู่แล้ว
```bash
# หา process ที่ใช้ port
lsof -i :8080
# Kill process
kill -9 <PID>
```

### Permission denied
```bash
# ใช้ port > 1024 หรือรันด้วย sudo
sudo ./builds/server.out 8080
```

### Connection timeout
- ตรวจสอบการตั้งค่า firewall
- ยืนยัน server IP และ port
- ให้แน่ใจว่า server ทำงานอยู่ก่อน client เชื่อมต่อ

### ไม่พบไฟล์ (File not found)
- ตรวจสอบว่าไฟล์มีอยู่ใน directory `example/` บน server
- ตรวจสอบการสะกดและตัวพิมพ์ใหญ่-เล็กของชื่อไฟล์

## หมายเหตุด้านประสิทธิภาพ (Performance Notes)

- **Throughput**: จำกัดโดย Stop-and-Wait ARQ (1 packet ต่อ RTT)
- **Packet Size**: payload 1024 bytes
- **Timeout**: 1 วินาที
- **Reliability**: 100% พร้อมการส่งซ้ำอัตโนมัติ

## License

โปรเจคการศึกษาสำหรับวิชา CS351 Networks and Clouds

## ผู้พัฒนา (Authors)

- Implementation:
   - นาย
   - นาย ไชยวัตน์ หนูวัฒนา รหัสนิสิต 6610401985
   - นาย
   - นาย
- Course: CS351 - Computer Communications and Cloud Computing Principles
