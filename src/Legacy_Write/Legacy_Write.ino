#include <SPI.h>          
#include <mcp2515.h>      
#include <Keypad.h>       

// ==== MCP2515 CAN Setup
struct can_frame cvtMsg;
struct can_frame engMsg;
struct can_frame meterMsg;
struct can_frame vsaMsg;

struct can_frame letter;
MCP2515 mcp2515(10);  // CS chân D10

// ==== Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'0','1','2','3'},
  {'4','5','6','7'},
  {'8','9','A','B'},
  {'C','D','E','F'}
};

byte rowPins[ROWS] = {3, 4, 5, 6};
byte colPins[COLS] = {7, 8, 9, A0};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Mảng lưu trạng thái các nút ấn: từ '0' đến '6'
bool bitState[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  Serial.begin(9600);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("CAN Transmitter with Keypad Ready");
}

void loop() {
  // ====== Đọc trạng thái phím ======
  for (int i = 0; i < 8; i++) bitState[i] = false;

  keypad.getKeys();
  for (int i = 0; i < LIST_MAX; i++) {
    if (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD) {
      char keyChar = keypad.key[i].kchar;
      if (keyChar >= '0' && keyChar <= '7') {
        bitState[keyChar - '0'] = true;
      }
    }
  }

  // ====== 1. CVT_191 — Điều khiển cần số (ID: 0x401) ======
  cvtMsg.can_id  = 0x191;
  cvtMsg.can_dlc = 8;
  for (int i = 0; i < 8; i++) cvtMsg.data[i] = 0x00;

  // bit0: P, bit1: R, bit2: N, bit3: D, bit4: S — chỉ một bit sẽ bật tại 1 thời điểm
  for (int i = 0; i < 4; i++) {
    if (bitState[i]) {
      cvtMsg.data[0] |= (1 << i);  // Set đúng bit cần thiết
    }
  }
  if(bitState[7]){
    cvtMsg.data[0] |= (1 << 4);
  }


  if (mcp2515.sendMessage(&cvtMsg) == MCP2515::ERROR_OK) {
    Serial.print("CVT_191 sent: bits 0–4 = ");
    Serial.println(cvtMsg.data[0] & 0x0F, BIN);
    Serial.print("CVT_191 data: ");
    for (int i = 0; i < cvtMsg.can_dlc; i++) {
      if (cvtMsg.data[i] < 0x10) Serial.print("0");
      Serial.print(cvtMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Error sending CVT_191");
  }

  // ====== 2. ENG_17C — Trạng thái động cơ và phanh chân ======
  engMsg.can_id  = 0x17C;
  engMsg.can_dlc = 8;
  for (int i = 0; i < 8; i++) engMsg.data[i] = 0x00;

  // Bit 16–31 → vòng tua máy (RPM)
  if (bitState[4]) {
    engMsg.data[2] = 0x05;
    engMsg.data[3] = 0x21;   // ~1313 RPM khi "bật máy"
  } else {
    engMsg.data[2] = 0x00;
    engMsg.data[3] = 0xDE;   // ~222 RPM khi "tắt máy"
  }

  if (mcp2515.sendMessage(&engMsg) == MCP2515::ERROR_OK) {
    Serial.print("ENG_17C sent: bit16–31 = ");
    uint16_t engBits16_31 = (engMsg.data[2] << 8) | engMsg.data[3];
    Serial.println(engBits16_31, HEX);
    
    Serial.print("ENG_17C sent: bit 32 = ");
    Serial.println((engMsg.data[4] >> 0) & 0x01);  // Lấy bit 0 của byte 4

    Serial.print("ENG_17C data: ");
    for (int i = 0; i < engMsg.can_dlc; i++) {
      if (engMsg.data[i] < 0x10) Serial.print("0");
      Serial.print(engMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Error sending ENG_17C");
  }

  // ====== 3. METER_1A6 — Phanh tay ======
  meterMsg.can_id = 0x1A6;
  meterMsg.can_dlc = 8;
  memset(meterMsg.data, 0, 8);
  // Bit 2 (byte 0) = 1 mặc định (phanh tay đang kéo)
  meterMsg.data[0] |= (1 << 2);
  // Nếu nhấn nút 6 (bitState[6]): thả phanh tay → bit 2 = 0
  if (bitState[6]) {
    meterMsg.data[0] &= ~(1 << 2);
  }
  if (mcp2515.sendMessage(&meterMsg) == MCP2515::ERROR_OK) {
    Serial.print("METER_1A6 sent, bit 2 = ");
    Serial.println((meterMsg.data[0] >> 2) & 0x01);
    Serial.print("METER_1A6 data: ");
    for (int i = 0; i < meterMsg.can_dlc; i++) {
      if (meterMsg.data[i] < 0x10) Serial.print("0");
      Serial.print(meterMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Error sending METER_1A6");
  }

  //====== 4.VSA_1A4 - Áp suất bàn đạp phanh =====    
  vsaMsg.can_id  = 0x1A4;
  vsaMsg.can_dlc = 8;
  for (int i = 0; i < 8; i++) vsaMsg.data[i] = 0x00;

  uint16_t rawPressure;
  if (bitState[5]) {
    rawPressure = (uint16_t)((407 + 2443.92) / 23.96); // 0x00 77 = dx 119
    Serial.print("Gửi phanh áp suất cao (đang đạp): ");
  } else {
    rawPressure = (uint16_t)((50.0 + 2443.92) / 23.96); // 0x00 68 ≈ dx 104.
    Serial.print("Gửi phanh áp suất thấp (không đạp): ");
  }

  // --- Mã hóa vào đúng bit: bit 0-3 + 8-15 ---
  vsaMsg.data[0] = 0x00;     // Ghi trực tiếp 8 bit vào byte đầu
  vsaMsg.data[1] = rawPressure;            // Nếu chưa dùng byte này

  Serial.print(rawPressure);
  Serial.print(" (");
  Serial.print(rawPressure * 23.96 - 2443.92);
  Serial.println(" kPa)");
  // Gửi CAN message
  if (mcp2515.sendMessage(&vsaMsg) == MCP2515::ERROR_OK) {
    Serial.println("VSA_1A4 message sent successfully.");
    Serial.print("VSA_1A4 data: ");
    for (int i = 0; i < vsaMsg.can_dlc; i++) {
      if (vsaMsg.data[i] < 0x10) Serial.print("0");
      Serial.print(vsaMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Error sending VSA_1A4 message.");
  }

  if (mcp2515.readMessage(&letter) == MCP2515::ERROR_OK) {
    // Kiểm tra có phải extended frame không
    if (letter.can_id & CAN_EFF_FLAG) {
      uint32_t id = letter.can_id & CAN_EFF_MASK;  // Lấy phần ID thực sự (29-bit)

      // Kiểm tra có phải ID cần đọc không
      if (id == 0x1234) {
        // Trích xuất 2 bit đầu tiên của data[0]
        uint8_t warning_level = letter.data[0] & 0x03;
        Serial.print("Received WARNING_ST: ");
        switch (warning_level) {
          case 0: Serial.println("0 - No Warning"); break;
          case 1: Serial.println("1 - First Level Warning"); break;
          case 2: Serial.println("2 - Second Level Warning"); break;
          case 3: Serial.println("3 - Emergency Warning"); break;
        }
      }
      else Serial.println("RECEIVING FROM HMIMSG FAILED.......");
    }
  }

}