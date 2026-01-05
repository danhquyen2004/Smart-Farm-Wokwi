#define ERA_AUTH_TOKEN "02bf896b-e6d7-4b64-a1e3-5098dda7f4a6"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TFT_eSPI.h>
#include <Adafruit_PWMServoDriver.h>
#include <ERaSimpleEsp32WiFi.hpp>
#include "ProfileManager.h"
#include "PlantZone.h"

const char ssid[] = "Wokwi-GUEST";
const char pass[] = "";

// Forward declarations
void TaskIoT(void * pvParameters);
void syncERa();

// Task handles
TaskHandle_t TaskIoTHandle = NULL;

// ==================== HARDWARE INSTANCES ====================

// OLED Display (I2C) - System Monitor
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// TFT Display (SPI) - Zone Monitoring
TFT_eSPI tft = TFT_eSPI();

// PCA9685 PWM Servo Driver (I2C)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// ==================== ZONES ====================

PlantZone* zone1 = nullptr;
PlantZone* zone2 = nullptr;

// ==================== TFT CS PIN CONTROL ====================

// CS pins for Zone displays (ESP32 DevKit V1)
const int PIN_CS_TFT1 = 15;  // Zone 1 Display
const int PIN_CS_TFT2 = 27;  // Zone 2 Display

void chonTFT(int soTFT) {
  // Bo chon tat ca TFT
  digitalWrite(PIN_CS_TFT1, HIGH);
  digitalWrite(PIN_CS_TFT2, HIGH);
  delayMicroseconds(10);
  
  switch (soTFT) {
    case 1: digitalWrite(PIN_CS_TFT1, LOW); break;
    case 2: digitalWrite(PIN_CS_TFT2, LOW); break;
  }
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  Serial.println("  Smart Farm v2.5 - E-RA IoT");
  Serial.println("  Main: OLED 128x64 (I2C)");
  Serial.println("  Zones: TFT ILI9341 (SPI)");
  Serial.println("  IoT: E-RA Platform");
  Serial.println("===========================================");
  
  // ====== Khoi tao E-RA & WiFi ======
  //Serial.println("\n[BUOC 0] Ket noi WiFi & E-RA...");
  //ERa.begin(ssid, pass);
  
  // Khoi tao I2C (can cho OLED va PCA9685)
  Wire.begin();
  delay(100);
  
  // ====== Khoi tao ProfileManager (SPIFFS) ======
  Serial.println("\n[BUOC 1] Khoi tao ProfileManager...");
  if (!profileManager.begin()) {
    Serial.println("[CANH BAO] ProfileManager khoi tao that bai, dung mac dinh");
  }
  
  // ====== Khoi tao chan TFT CS ======
  pinMode(PIN_CS_TFT1, OUTPUT);
  pinMode(PIN_CS_TFT2, OUTPUT);
  digitalWrite(PIN_CS_TFT1, HIGH);
  digitalWrite(PIN_CS_TFT2, HIGH);
  
  // ====== Khoi tao PCA9685 ======
  Serial.println("\n[BUOC 2] Khoi tao PCA9685...");
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);
  Serial.println("[OK] PCA9685 da khoi tao");
  
  // ====== Khoi tao OLED Display ======
  Serial.println("\n[BUOC 3] Khoi tao OLED...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("[LOI] Khoi tao SSD1306 that bai"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("SMART FARM v2.4");
    display.println("-------------------");
    display.println("He thong: San sang");
    display.println("So vung: 2");
    display.printf("Profiles: %d\n", profileManager.getSoLuongProfile());
    display.println("-------------------");
    display.println("Dang khoi tao...");
    display.display();
    Serial.println("[OK] OLED da khoi tao");
  }
  
  // ====== Khoi tao TFT Displays ======
  Serial.println("\n[BUOC 4] Khoi tao TFT...");
  chonTFT(1); 
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(false);
  delay(100);
  
  // Xoa man hinh Vung 1
  chonTFT(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Khoi tao Vung 1...", 10, 10);
  delay(100);
  
  // Xoa man hinh Vung 2
  chonTFT(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Khoi tao Vung 2...", 10, 10);
  delay(100);
  
  Serial.println("[OK] TFT da khoi tao");
  
  // ====== Khoi tao cac Vung trong ======
  Serial.println("\n[BUOC 5] Khoi tao cac Vung trong...");
  
  zone1 = new PlantZone(0, &tft, &pwm);
  zone1->begin();
  
  zone2 = new PlantZone(1, &tft, &pwm);
  zone2->begin();
  
  // Gan cau hinh tu ProfileManager
  PlantProfile* profile1 = profileManager.getProfile(0);  // Xa Lach
  PlantProfile* profile2 = profileManager.getProfile(1);  // Dau Tay
  
  if (profile1) {
    zone1->setCauHinh(profile1);
    Serial.printf("Vung 1: %s\n", profile1->ten);
  } else {
    Serial.println("[CANH BAO] Khong tim thay profile cho Vung 1");
  }
  
  if (profile2) {
    zone2->setCauHinh(profile2);
    Serial.printf("Vung 2: %s\n", profile2->ten);
  } else {
    Serial.println("[CANH BAO] Khong tim thay profile cho Vung 2");
  }
  
  // Cap nhat OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("SMART FARM v2.4");
  display.println("-------------------");
  display.println("He thong: San sang");
  display.printf("V1: %s\n", profile1 ? profile1->ten : "N/A");
  display.printf("V2: %s\n", profile2 ? profile2->ten : "N/A");
  display.println("-------------------");
  display.println("Dang chay...");
  display.display();
  
  Serial.println("\n[OK] Tat ca he thong da khoi tao");
  Serial.println("===========================================\n");

  // ====== Khoi tao Task IoT tren Core 0 ======
  xTaskCreatePinnedToCore(
    TaskIoT,            // Ham thuc thi task
    "TaskIoT",          // Ten task
    10000,              // Stack size (byte)
    NULL,               // Tham so
    1,                  // Uu tien (thap hon logic chinh)
    &TaskIoTHandle,     // Handle
    0                   // Chay tren Core 0
  );
}

// ==================== IOT TASK (Core 0) ====================
void TaskIoT(void * pvParameters) {
  Serial.print("[Core 0] Task IoT dang chay tren nhan: ");
  Serial.println(xPortGetCoreID());

  // Khoi tao ERa ben trong task
  ERa.begin(ssid, pass);

  for(;;) {
    ERa.run();
    syncERa();
    vTaskDelay(10 / portTICK_PERIOD_MS); // Nhuong thoi gian cho system
  }
}

// ==================== ERa VIRTUAL PIN HANDLERS ====================

// Zone 1 Handlers
ERA_WRITE(V12) { // Auto Mode
    bool val = param.getInt();
    if (zone1) zone1->setCheTuDong(val);
}
ERA_WRITE(V9)  { if (zone1) zone1->batTatQuat(param.getInt()); }
ERA_WRITE(V10) { if (zone1) zone1->batTatBomNuoc(param.getInt()); }
ERA_WRITE(V11) { if (zone1) zone1->batTatDen(param.getInt()); }
ERA_WRITE(V18) { if (zone1) zone1->batTatSuoi(param.getInt()); }
ERA_WRITE(V19) { if (zone1) zone1->batTatPhunSuong(param.getInt()); }
ERA_WRITE(V14) { if (zone1) zone1->batTatBomDinhDuong(param.getInt()); }
ERA_WRITE(V15) { if (zone1) zone1->batTatBomN(param.getInt()); }
ERA_WRITE(V16) { if (zone1) zone1->batTatBomP(param.getInt()); }
ERA_WRITE(V17) { if (zone1) zone1->batTatBomK(param.getInt()); }

ERA_WRITE(V40) { // Temp Max
    if (zone1 && zone1->getCauHinh()) {
        zone1->getCauHinh()->nhietDoMax = param.getFloat();
        profileManager.saveProfiles();
    }
}
ERA_WRITE(V41) { // Temp Min
    if (zone1 && zone1->getCauHinh()) {
        zone1->getCauHinh()->nhietDoMin = param.getFloat();
        profileManager.saveProfiles();
    }
}
ERA_WRITE(V43) { // Soil Moist Min
    if (zone1 && zone1->getCauHinh()) {
        zone1->getCauHinh()->doAmDatMin = param.getInt();
        profileManager.saveProfiles();
    }
}

// Zone 2 Handlers
ERA_WRITE(V32) { // Auto Mode
    bool val = param.getInt();
    if (zone2) zone2->setCheTuDong(val);
}
ERA_WRITE(V29) { if (zone2) zone2->batTatQuat(param.getInt()); }
ERA_WRITE(V30) { if (zone2) zone2->batTatBomNuoc(param.getInt()); }
ERA_WRITE(V31) { if (zone2) zone2->batTatDen(param.getInt()); }
ERA_WRITE(V38) { if (zone2) zone2->batTatSuoi(param.getInt()); }
ERA_WRITE(V39) { if (zone2) zone2->batTatPhunSuong(param.getInt()); }
ERA_WRITE(V34) { if (zone2) zone2->batTatBomDinhDuong(param.getInt()); }
ERA_WRITE(V35) { if (zone2) zone2->batTatBomN(param.getInt()); }
ERA_WRITE(V36) { if (zone2) zone2->batTatBomP(param.getInt()); }
ERA_WRITE(V37) { if (zone2) zone2->batTatBomK(param.getInt()); }

ERA_WRITE(V60) { // Temp Max
    if (zone2 && zone2->getCauHinh()) {
        zone2->getCauHinh()->nhietDoMax = param.getFloat();
        profileManager.saveProfiles();
    }
}
ERA_WRITE(V61) { // Temp Min
    if (zone2 && zone2->getCauHinh()) {
        zone2->getCauHinh()->nhietDoMin = param.getFloat();
        profileManager.saveProfiles();
    }
}
ERA_WRITE(V63) { // Soil Moist Min
    if (zone2 && zone2->getCauHinh()) {
        zone2->getCauHinh()->doAmDatMin = param.getInt();
        profileManager.saveProfiles();
    }
}

// Push data to ERa - Staggered implementation to prevent lag
void syncERa() {
    static unsigned long lastStep = 0;
    static int step = 0;
    const int totalSteps = 46; 
    
    if (millis() - lastStep < 150) return; 
    lastStep = millis();

    switch (step) {
        // --- ZONE 1 (0-19) ---
        case 0: if (zone1) ERa.virtualWrite(V0, zone1->getNhietDo()); break;
        case 1: if (zone1) ERa.virtualWrite(V1, zone1->getDoAmKK()); break;
        case 2: if (zone1) ERa.virtualWrite(V2, zone1->getAnhSang()); break;
        case 3: if (zone1) ERa.virtualWrite(V3, zone1->getDoAmDat()); break;
        case 4: if (zone1) ERa.virtualWrite(V4, zone1->getPHDat()); break;
        case 5: if (zone1) ERa.virtualWrite(V5, zone1->getN()); break;
        case 6: if (zone1) ERa.virtualWrite(V6, zone1->getP()); break;
        case 7: if (zone1) ERa.virtualWrite(V7, zone1->getK()); break;
        case 8: if (zone1) ERa.virtualWrite(V8, zone1->getEC()); break;
        case 9:  if (zone1) ERa.virtualWrite(V9, zone1->laQuatChay()); break;
        case 10: if (zone1) ERa.virtualWrite(V10, zone1->laBomNuocChay()); break;
        case 11: if (zone1) ERa.virtualWrite(V11, zone1->laDenChay()); break;
        case 12: if (zone1) ERa.virtualWrite(V12, zone1->getCheTuDong()); break;
        case 13: if (zone1) ERa.virtualWrite(V13, zone1->laCanhBao()); break;
        case 14: if (zone1) ERa.virtualWrite(V14, zone1->laBomDinhDuongChay()); break;
        case 15: if (zone1) ERa.virtualWrite(V15, zone1->laBomNChay()); break;
        case 16: if (zone1) ERa.virtualWrite(V16, zone1->laBomPChay()); break;
        case 17: if (zone1) ERa.virtualWrite(V17, zone1->laBomKChay()); break;
        case 18: if (zone1) ERa.virtualWrite(V18, zone1->laSuoiChay()); break;
        case 19: if (zone1) ERa.virtualWrite(V19, zone1->laPhunSuongChay()); break;

        // --- ZONE 2 (20-39) ---
        case 20: if (zone2) ERa.virtualWrite(V20, zone2->getNhietDo()); break;
        case 21: if (zone2) ERa.virtualWrite(V21, zone2->getDoAmKK()); break;
        case 22: if (zone2) ERa.virtualWrite(V22, zone2->getAnhSang()); break;
        case 23: if (zone2) ERa.virtualWrite(V23, zone2->getDoAmDat()); break;
        case 24: if (zone2) ERa.virtualWrite(V24, zone2->getPHDat()); break;
        case 25: if (zone2) ERa.virtualWrite(V25, zone2->getN()); break;
        case 26: if (zone2) ERa.virtualWrite(V26, zone2->getP()); break;
        case 27: if (zone2) ERa.virtualWrite(V27, zone2->getK()); break;
        case 28: if (zone2) ERa.virtualWrite(V28, zone2->getEC()); break;
        case 29: if (zone2) ERa.virtualWrite(V29, zone2->laQuatChay()); break;
        case 30: if (zone2) ERa.virtualWrite(V30, zone2->laBomNuocChay()); break;
        case 31: if (zone2) ERa.virtualWrite(V31, zone2->laDenChay()); break;
        case 32: if (zone2) ERa.virtualWrite(V32, zone2->getCheTuDong()); break;
        case 33: if (zone2) ERa.virtualWrite(V33, zone2->laCanhBao()); break;
        case 34: if (zone2) ERa.virtualWrite(V34, zone2->laBomDinhDuongChay()); break;
        case 35: if (zone2) ERa.virtualWrite(V35, zone2->laBomNChay()); break;
        case 36: if (zone2) ERa.virtualWrite(V36, zone2->laBomPChay()); break;
        case 37: if (zone2) ERa.virtualWrite(V37, zone2->laBomKChay()); break;
        case 38: if (zone2) ERa.virtualWrite(V38, zone2->laSuoiChay()); break;
        case 39: if (zone2) ERa.virtualWrite(V39, zone2->laPhunSuongChay()); break;

        // --- THRESHOLDS (40-45) ---
        case 40: if (zone1 && zone1->getCauHinh()) ERa.virtualWrite(V40, zone1->getCauHinh()->nhietDoMax); break;
        case 41: if (zone1 && zone1->getCauHinh()) ERa.virtualWrite(V41, zone1->getCauHinh()->nhietDoMin); break;
        case 42: if (zone1 && zone1->getCauHinh()) ERa.virtualWrite(V43, zone1->getCauHinh()->doAmDatMin); break;
        case 43: if (zone2 && zone2->getCauHinh()) ERa.virtualWrite(V60, zone2->getCauHinh()->nhietDoMax); break;
        case 44: if (zone2 && zone2->getCauHinh()) ERa.virtualWrite(V61, zone2->getCauHinh()->nhietDoMin); break;
        case 45: if (zone2 && zone2->getCauHinh()) ERa.virtualWrite(V63, zone2->getCauHinh()->doAmDatMin); break;
    }

    step++;
    if (step >= totalSteps) step = 0;
}


// ==================== MAIN LOOP ====================

void loop() {
  // Loop mac dinh chay tren Core 1
  
  // Cap nhat Vung 1
  chonTFT(1);
  if (zone1 != nullptr) zone1->update(0);
  
  // Cap nhat Vung 2
  chonTFT(2);
  if (zone2 != nullptr) zone2->update(0);
  
  delay(50);
}
