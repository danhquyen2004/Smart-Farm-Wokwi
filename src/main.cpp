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

// Zone 1 Handlers (Thresholds V40-V51)
ERA_WRITE(V12) { bool val = param.getInt(); if (zone1) zone1->setCheTuDong(val); }
ERA_WRITE(V9)  { if (zone1) zone1->batTatQuat(param.getInt()); }
ERA_WRITE(V10) { if (zone1) zone1->batTatBomNuoc(param.getInt()); }
ERA_WRITE(V11) { if (zone1) zone1->batTatDen(param.getInt()); }
ERA_WRITE(V18) { if (zone1) zone1->batTatSuoi(param.getInt()); }
ERA_WRITE(V19) { if (zone1) zone1->batTatPhunSuong(param.getInt()); }
ERA_WRITE(V14) { if (zone1) zone1->batTatBomDinhDuong(param.getInt()); }
ERA_WRITE(V15) { if (zone1) zone1->batTatBomN(param.getInt()); }
ERA_WRITE(V16) { if (zone1) zone1->batTatBomP(param.getInt()); }
ERA_WRITE(V17) { if (zone1) zone1->batTatBomK(param.getInt()); }

ERA_WRITE(V40) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->nhietDoMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V41) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->nhietDoMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V42) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->doAmKKMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V43) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->doAmDatMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V44) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->anhSangMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V45) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->phMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V46) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->phMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V47) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->nMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V48) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->pMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V49) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->kMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V50) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->ecMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V51) { if (zone1 && zone1->getCauHinh()) { zone1->getCauHinh()->ecMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V52) { if (zone1 && zone1->getCauHinh()) { strncpy(zone1->getCauHinh()->ten, param.getString(), 29); zone1->getCauHinh()->ten[29] = '\0'; profileManager.saveProfiles(); } }

// Zone 2 Handlers (Thresholds V60-V71)
ERA_WRITE(V32) { bool val = param.getInt(); if (zone2) zone2->setCheTuDong(val); }
ERA_WRITE(V29) { if (zone2) zone2->batTatQuat(param.getInt()); }
ERA_WRITE(V30) { if (zone2) zone2->batTatBomNuoc(param.getInt()); }
ERA_WRITE(V31) { if (zone2) zone2->batTatDen(param.getInt()); }
ERA_WRITE(V38) { if (zone2) zone2->batTatSuoi(param.getInt()); }
ERA_WRITE(V39) { if (zone2) zone2->batTatPhunSuong(param.getInt()); }
ERA_WRITE(V34) { if (zone2) zone2->batTatBomDinhDuong(param.getInt()); }
ERA_WRITE(V35) { if (zone2) zone2->batTatBomN(param.getInt()); }
ERA_WRITE(V36) { if (zone2) zone2->batTatBomP(param.getInt()); }
ERA_WRITE(V37) { if (zone2) zone2->batTatBomK(param.getInt()); }

ERA_WRITE(V60) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->nhietDoMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V61) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->nhietDoMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V62) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->doAmKKMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V63) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->doAmDatMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V64) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->anhSangMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V65) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->phMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V66) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->phMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V67) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->nMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V68) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->pMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V69) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->kMin = param.getInt(); profileManager.saveProfiles(); } }
ERA_WRITE(V70) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->ecMin = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V71) { if (zone2 && zone2->getCauHinh()) { zone2->getCauHinh()->ecMax = param.getFloat(); profileManager.saveProfiles(); } }
ERA_WRITE(V72) { if (zone2 && zone2->getCauHinh()) { strncpy(zone2->getCauHinh()->ten, param.getString(), 29); zone2->getCauHinh()->ten[29] = '\0'; profileManager.saveProfiles(); } }

// Push data to ERa - Burst Mode for Realtime experience
void syncERa() {
    static unsigned long lastFastSync = 0;
    static unsigned long lastSlowSync = 0;

    // 1. NHOM REAL-TIME (Cam bien & Thiet bi) - Cap nhat moi 500ms
    if (millis() - lastFastSync >= 500) {
        lastFastSync = millis();
        
        if (zone1) {
            // Zone 1 Sensors
            ERa.virtualWrite(V0, zone1->getNhietDo());
            ERa.virtualWrite(V1, zone1->getDoAmKK());
            ERa.virtualWrite(V2, zone1->getAnhSang());
            ERa.virtualWrite(V3, zone1->getDoAmDat());
            ERa.virtualWrite(V4, zone1->getPHDat());
            ERa.virtualWrite(V5, zone1->getN());
            ERa.virtualWrite(V6, zone1->getP());
            ERa.virtualWrite(V7, zone1->getK());
            ERa.virtualWrite(V8, zone1->getEC());
            // Zone 1 Actuators
            ERa.virtualWrite(V9, zone1->laQuatChay());
            ERa.virtualWrite(V10, zone1->laBomNuocChay());
            ERa.virtualWrite(V11, zone1->laDenChay());
            ERa.virtualWrite(V12, zone1->getCheTuDong());
            ERa.virtualWrite(V13, zone1->laCanhBao());
            ERa.virtualWrite(V14, zone1->laBomDinhDuongChay());
            ERa.virtualWrite(V15, zone1->laBomNChay());
            ERa.virtualWrite(V16, zone1->laBomPChay());
            ERa.virtualWrite(V17, zone1->laBomKChay());
            ERa.virtualWrite(V18, zone1->laSuoiChay());
            ERa.virtualWrite(V19, zone1->laPhunSuongChay());
        }

        if (zone2) {
            // Zone 2 Sensors
            ERa.virtualWrite(V20, zone2->getNhietDo());
            ERa.virtualWrite(V21, zone2->getDoAmKK());
            ERa.virtualWrite(V22, zone2->getAnhSang());
            ERa.virtualWrite(V23, zone2->getDoAmDat());
            ERa.virtualWrite(V24, zone2->getPHDat());
            ERa.virtualWrite(V25, zone2->getN());
            ERa.virtualWrite(V26, zone2->getP());
            ERa.virtualWrite(V27, zone2->getK());
            ERa.virtualWrite(V28, zone2->getEC());
            // Zone 2 Actuators
            ERa.virtualWrite(V29, zone2->laQuatChay());
            ERa.virtualWrite(V30, zone2->laBomNuocChay());
            ERa.virtualWrite(V31, zone2->laDenChay());
            ERa.virtualWrite(V32, zone2->getCheTuDong());
            ERa.virtualWrite(V33, zone2->laCanhBao());
            ERa.virtualWrite(V34, zone2->laBomDinhDuongChay());
            ERa.virtualWrite(V35, zone2->laBomNChay());
            ERa.virtualWrite(V36, zone2->laBomPChay());
            ERa.virtualWrite(V37, zone2->laBomKChay());
            ERa.virtualWrite(V38, zone2->laSuoiChay());
            ERa.virtualWrite(V39, zone2->laPhunSuongChay());
        }
    }

    // 2. NHOM CAU HINH (Nguong ly tuong) - Cap nhat moi 5 giay (5000ms)
    if (millis() - lastSlowSync >= 5000) {
        lastSlowSync = millis();
        
        if (zone1 && zone1->getCauHinh()) {
            PlantProfile* p = zone1->getCauHinh();
            ERa.virtualWrite(V40, p->nhietDoMax);
            ERa.virtualWrite(V41, p->nhietDoMin);
            ERa.virtualWrite(V42, p->doAmKKMin);
            ERa.virtualWrite(V43, p->doAmDatMin);
            ERa.virtualWrite(V44, p->anhSangMin);
            ERa.virtualWrite(V45, p->phMin);
            ERa.virtualWrite(V46, p->phMax);
            ERa.virtualWrite(V47, p->nMin);
            ERa.virtualWrite(V48, p->pMin);
            ERa.virtualWrite(V50, p->ecMin);
            ERa.virtualWrite(V51, p->ecMax);
            ERa.virtualWrite(V52, p->ten);
        }
        
        if (zone2 && zone2->getCauHinh()) {
            PlantProfile* p = zone2->getCauHinh();
            ERa.virtualWrite(V60, p->nhietDoMax);
            ERa.virtualWrite(V61, p->nhietDoMin);
            ERa.virtualWrite(V62, p->doAmKKMin);
            ERa.virtualWrite(V63, p->doAmDatMin);
            ERa.virtualWrite(V64, p->anhSangMin);
            ERa.virtualWrite(V65, p->phMin);
            ERa.virtualWrite(V66, p->phMax);
            ERa.virtualWrite(V67, p->nMin);
            ERa.virtualWrite(V68, p->pMin);
            ERa.virtualWrite(V70, p->ecMin);
            ERa.virtualWrite(V71, p->ecMax);
            ERa.virtualWrite(V72, p->ten);
        }
    }
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
