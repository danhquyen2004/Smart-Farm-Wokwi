#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TFT_eSPI.h>
#include <Adafruit_PWMServoDriver.h>
#include "MuxManager.h"
#include "PlantZone.h"

// ==================== HARDWARE INSTANCES ====================

// OLED Display (I2C) - System Monitor
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// TFT Display (SPI) - Zone Monitoring
TFT_eSPI tft = TFT_eSPI();

// PCA9685 PWM Servo Driver (I2C)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Multiplexer Manager (MUX1 for shared sensors)
MuxManager mux;

// ==================== ZONES ====================

PlantZone* zone1 = nullptr;
PlantZone* zone2 = nullptr;

// ==================== UI ====================

int currentProfileIndex = 0;
PlantProfile* profiles[] = { &PROFILE_LETTUCE, &PROFILE_STRAWBERRY, &PROFILE_TOMATO };
const int NUM_PROFILES = 3;

int selectedZoneBtn = 0;

// ==================== TFT CS PIN CONTROL ====================

// CS pins for Zone displays (ESP32 DevKit V1)
const int PIN_CS_TFT1 = 15;  // Zone 1 Display
const int PIN_CS_TFT2 = 27;  // Zone 2 Display

void selectTFT(int tftNum) {
  // Deselect all known TFTs
  digitalWrite(PIN_CS_TFT1, HIGH);
  digitalWrite(PIN_CS_TFT2, HIGH);
  delayMicroseconds(10);
  
  switch (tftNum) {
    case 1: digitalWrite(PIN_CS_TFT1, LOW); break;
    case 2: digitalWrite(PIN_CS_TFT2, LOW); break;
  }
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  Serial.println("\n===========================================");
  Serial.println("  Smart Farm v2.3 - Shared Environment");
  Serial.println("  Main: OLED 128x64 (I2C)");
  Serial.println("  Zones: TFT ILI9341 (SPI)");
  Serial.println("  Sensors: Shared for both zones");
  Serial.println("===========================================");
  
  // Initialize I2C (needed for OLED and PCA9685)
  // Pins SDA=21, SCL=22 default
  Wire.begin();
  
  // Initialize I2C
  Wire.begin();
  delay(100);
  
  // Initialize TFT CS pins
  pinMode(PIN_CS_TFT1, OUTPUT);
  pinMode(PIN_CS_TFT2, OUTPUT);
  digitalWrite(PIN_CS_TFT1, HIGH);
  digitalWrite(PIN_CS_TFT2, HIGH);
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);
  Serial.println("[OK] PCA9685 initialized");
  
  // Initialize MUX
  // mux.begin();
  // delay(100);
  
  // ====== Initialize OLED Display (System Monitor) ======
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("[ERR] SSD1306 allocation failed"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("SMART FARM v2.3");
    display.println("-------------------");
    display.println("System: Ready");
    display.println("Zones: 2");
    display.println("Sensors: Shared");
    display.println("-------------------");
    display.println("Login via App");
    display.display();
    Serial.println("[OK] OLED Display initialized");
  }
  
  // ====== Initialize TFT Displays (SPI) ======
  selectTFT(1); 
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(false); // Fix white background/color swap
  delay(100);
  
  // Clear Zone 1 Display
  selectTFT(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Khoi tao Vung 1...", 10, 10);
  delay(100);
  
  // Clear Zone 2 Display
  selectTFT(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Khoi tao Vung 2...", 10, 10);
  delay(100);
  
  Serial.println("[OK] TFT Displays initialized and cleared");
  
  // ====== Initialize Zones ======
  Serial.println("Initializing Plant Zones...");
  
  zone1 = new PlantZone(0, &tft, &pwm, &mux);
  zone1->begin();
  zone1->setProfile(&PROFILE_LETTUCE);
  
  zone2 = new PlantZone(1, &tft, &pwm, &mux);
  zone2->begin();
  zone2->setProfile(&PROFILE_STRAWBERRY);
  
  Serial.println("[OK] All systems initialized");
  Serial.println("===========================================\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  // Update Zone 1 TFT
  selectTFT(1);
  if (zone1 != nullptr) zone1->update(0);
  
  // Update Zone 2 TFT
  selectTFT(2);
  if (zone2 != nullptr) zone2->update(0);
  
  delay(100);
}
