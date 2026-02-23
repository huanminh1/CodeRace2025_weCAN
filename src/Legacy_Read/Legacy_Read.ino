#include <U8glib.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>

U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE); 

// =======================
struct can_frame hmiMsg;


int gearState = 0;  // 0 = P, 1 = R, 2 = N, 3 = D; 4 = S

bool engineOn = false;
int engineSpeed = 0;      //tốc độ động cơ, threshold là 300rpm. speed < 300rpm gần như engine đã off
bool pedalBrake = 0;    //=0 là đang không phanh / = 1 là đang phanh. 
bool parkBrake = 1;        //=0 là phanh tay đang OFF / = 1 là phanh tay đang ON
bool lock = true;
bool warning = 0;       //dieu kien hien warning
unsigned long lastDisplayTime = 0;
String displayMessage1 = "";
String displayMessage2 = "";
String displayMessage3 = "";
String gearMessage ="";
String engineMessage="Engine OFF";
// =======================

struct can_frame canMsg;
MCP2515 mcp2515(10);  // CS = D10

void setup() {
  Serial.begin(9600);
  //Serial.begin(115200);


  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  u8g.firstPage();
  do {} while (u8g.nextPage());
  Serial.println("CAN Receiver Ready...");
}
void drawOLED() {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    
    // Luôn hiển thị trạng thái số
    u8g.drawStr(0, 12, gearMessage.c_str());
    u8g.drawStr(0, 24, engineMessage.c_str());
    
    // Chỉ hiển thị cảnh báo nếu có nội dung
    if (displayMessage1 != "") {
      u8g.drawStr(0, 36, displayMessage1.c_str());
      u8g.drawStr(0, 48, displayMessage2.c_str());
      u8g.drawStr(0, 60, displayMessage3.c_str());
    }

  } while (u8g.nextPage());
}
void loop() {
  unsigned long now = millis(); 
  bool gotCVT = false;
  bool gotENG = false;

  hmiMsg.can_id  = 0x1234 | CAN_EFF_FLAG; // Extended ID + flag
  hmiMsg.can_dlc = 8; // Dữ liệu có 1 byte
  hmiMsg.data[0] = 0x01;  //const message

  // Đọc toàn bộ message đang có
  while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    // ======= DÒNG 1: IN DỮ LIỆU DẠNG BIT CHUẨN BIG-ENDIAN =======
    if (canMsg.can_id == 0x191) {
      Serial.print("CVT Bits:  ");
      gotCVT = true;
          for (int i = 0; i < 8; i++) {
            if (canMsg.data[i] < 0x10) Serial.print("0"); // thêm số 0 phía trước nếu < 0x10
            Serial.print(canMsg.data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();  // xuống dòng
    } else if (canMsg.can_id == 0x17C) {
      Serial.print("ENG Bits:  ");
      gotENG = true;
            for (int i = 0; i < 8; i++) {
            if (canMsg.data[i] < 0x10) Serial.print("0"); // thêm số 0 phía trước nếu < 0x10
            Serial.print(canMsg.data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();  // xuống dòng
    } else if (canMsg.can_id == 0x1A6){
      Serial.print("METER BITS: ");
          for (int i = 0; i < 8; i++) {
            if (canMsg.data[i] < 0x10) Serial.print("0"); // thêm số 0 phía trước nếu < 0x10
            Serial.print(canMsg.data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();  // xuống dòng 
    } else if (canMsg.can_id == 0x1A4){
      Serial.print("VSA Bits: ");
            for (int i = 0; i < 8; i++) {
            if (canMsg.data[i] < 0x10) Serial.print("0"); // thêm số 0 phía trước nếu < 0x10
            Serial.print(canMsg.data[i], HEX);
            Serial.print(" ");
          }
          Serial.println();  // xuống dòng
    } 

    // ======= DÒNG 2: CẬP NHẬT TRẠNG THÁI TỪ MESSAGE =======
  if (canMsg.can_id == 0x191) {
    bool bit0 = bitRead(canMsg.data[0], 0); // P
    bool bit1 = bitRead(canMsg.data[0], 1); // R
    bool bit2 = bitRead(canMsg.data[0], 2); // N
    bool bit3 = bitRead(canMsg.data[0], 3); // D
    bool bit4 = bitRead(canMsg.data[0], 4); // S

      // Nếu có nút được nhấn
    if (bit0 || bit1 || bit2 || bit3 || bit4) {          
      if (bit0) {
        gearState = 0;
        gearMessage = "Gear: P (Parking) ";
      }
      else if (bit1) {
        gearState = 1;
        gearMessage = "Gear: R (Reverse)";
      }
      else if (bit2) {
        gearState = 2;
        gearMessage = "Gear: N (Neutral)";
      }
      else if (bit3) {
        gearState = 3;
        gearMessage = "Gear: D (Drive)";
      }
      else if (bit4) {
        gearState = 4;
        gearMessage = "Gear: S (Sport) ";
      }
      drawOLED();  // Chỉ cập nhật OLED khi có sự thay đổi
    }
  // Ngược lại: không làm gì, giữ nguyên trạng thái
  }

  if (canMsg.can_id == 0x17C) {
    // 1. Đọc 2 byte từ bit 16–31 → data[2] và data[3]
    uint16_t rawSpeed = ((uint16_t)canMsg.data[2] << 8) | canMsg.data[3];
    engineSpeed = rawSpeed;

    // 2. Cập nhật engineOn theo ngưỡng 400 rpm
    if (engineSpeed >= 400) {   //**************
      engineOn = true;
    } else {
      engineOn = false;
    }

    // // 3. Đọc bit 32 (bit số 0 của data[4]) để xác định có đạp phanh hay không
    // pedalBrakeNo = (canMsg.data[4] & 0x01) ? 1 : 0;

    // 4. Cập nhật nội dung hiển thị OLED
    if (engineOn) {
      engineMessage = "Engine ON";
    } else {
      engineMessage = "Engine OFF";
    }

    drawOLED();

    // 5. In thông tin ra Serial để debug
    Serial.print("Engine speed (rpm): ");
    Serial.println(engineSpeed);
    Serial.print("Engine On: ");
    Serial.println(engineOn);
    // Serial.print("Pedal Brake: Hold (1 = braking): ");
    // Serial.println(pedalBrakeNo);
  }
  if (canMsg.can_id == 0x1A6) {
    // Đọc bit số 2 trong message (bit thứ 2 trong tổng 64 bit, tức là byte 0, bit 2)
    parkBrake = (canMsg.data[0] >> 2) & 0x01;

    // Cập nhật biến parkBrake
    // In ra Serial để debug
    if (parkBrake) {
      Serial.println("Phanh tay ON");
    } else {
      Serial.println("Phanh tay OFF");
    }
  }
  if (canMsg.can_id == 0x1A4) {
    // Tái tạo giá trị áp suất phanh từ data[0] (bit 0–3) và data[1] (bit 8–15)
    uint16_t rawPressure =  ((canMsg.data[0] & 0x0F)<<8) |canMsg.data[1] ;
    float brakePressure = rawPressure *  23.96 - 2443.92;
    // Kiểm tra ngưỡng để xác định có đang đạp phanh hay không
    if (brakePressure > 100) {
      pedalBrake = 1;
    } else {
      pedalBrake = 0;
    }

    // In ra để debug
    Serial.print("raw: ");
    Serial.println(rawPressure);
    Serial.print("Pressure: ");
    Serial.print(brakePressure);
    Serial.print(" kPa | pedalBrake: ");
    Serial.println(pedalBrake);
  }
}

  if(lock){
    // Sau 3 giây thì xóa cảnh báo
    if (now - lastDisplayTime > 3000  && displayMessage1 != "") {
      Serial.println("Clearing OLED warning...");
      displayMessage1 = "";
      displayMessage2 = "";
      displayMessage3 = "";
      lock = true;
      drawOLED();
    }

    /////////////////////////////////////////////////////////
    //////////////ENGINE ON//////////////////////////////
    ////////////////////////////////////////////////////////
    if (engineOn && gearState == 0 && parkBrake) {
      displayMessage1 = "Safety";
      displayMessage2 = "";
      displayMessage3 = "";
      drawOLED();
      lock = 0;
      //warning = true;
      lastDisplayTime = now;
    }
    else if (engineOn && gearState == 0 && !parkBrake){
      displayMessage1 = ">> Engage the ";
      displayMessage2 = " Parking Brake";
      displayMessage3 = "";  
      drawOLED();
      lock = 0;
      warning = true;
      lastDisplayTime = now;
    }
    else if (engineOn && gearState != 0 && parkBrake){
      displayMessage1 = ">>>Hold brake";
      displayMessage2 = "and Release";
      displayMessage3 = "Parking Brake ";
      drawOLED();
      lock = 0;
      warning = true;
      lastDisplayTime = now;
    }
    else if(engineOn && gearState != 0 && !parkBrake ){
      displayMessage1 = "Driving";
      displayMessage2 = "";
      displayMessage3 = "";
      drawOLED();
      lock = 0;
      // warning = true;
      lastDisplayTime = now;
    }
    /////////////////////////////////////////////////////////
    //////////////ENGINE OFF//////////////////////////////
    ////////////////////////////////////////////////////////
    else if (!engineOn && gearState == 0 && !parkBrake) {
      displayMessage1 = ">> Engage the Parking";
      displayMessage2 = "Brake";
      displayMessage3 = "";
      drawOLED();
      lock = 0;
      warning = true;
      lastDisplayTime = now;
    }
    else if (!engineOn && gearState != 0 && parkBrake) {
      displayMessage1 = "Shift gear to P";
      displayMessage2 = "";
      displayMessage3 = "";
      drawOLED();
      lock = 0;
      warning = true;
      lastDisplayTime = now;
    }
    else if (!engineOn && gearState != 0 && !parkBrake && !pedalBrake) {
      displayMessage1 = "";
      displayMessage2 = ">>> Shift to P or";
      displayMessage3 = ">>> Hold Brake";
      drawOLED();
      lock = 0;
      warning = true;
      lastDisplayTime = now;
    }
  }

// Sau 3 giây thì xóa cảnh báo 
  if (now - lastDisplayTime > 3000  && displayMessage1 != "") {
    Serial.println("Clearing OLED warning...");
    lock = true;
    warning = false;
  }
  if(warning){
    // sendCAN;
    if (mcp2515.sendMessage(&hmiMsg) == MCP2515::ERROR_OK) {
      // Serial.println(" Send to HMI: SUCCESS");
      uint32_t id = hmiMsg.can_id & CAN_EFF_MASK; // Lấy phần ID thật sự (bỏ flag)
      Serial.print(" ID: 0x"); Serial.println(id, HEX);
      Serial.print("Data: ");
      for (int i = 0; i < hmiMsg.can_dlc; i++) {
        if (hmiMsg.data[i] < 0x10) Serial.print("0");
        Serial.print(hmiMsg.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    else {
      Serial.println(" Send to HMI: FAILED");
    }
  }
}
