#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_PWMServoDriver.h>
#include "ZoneA.h"
#include "ZoneB.h"

// TFT display
TFT_eSPI tft = TFT_eSPI();

// PCA9685 servo driver (I2C address 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Zone instances
ZoneA* zoneA = nullptr;
ZoneB* zoneB = nullptr;

// Button for profile switching
const int PIN_BUTTON = 0;  // MODE button from diagram.json
int currentProfileIndex = 0;
PlantProfile* profiles[] = { &PROFILE_LETTUCE, &PROFILE_MELON, &PROFILE_STRAWBERRY };
const char* profileNames[] = { "XA LACH", "DUA LUOI", "DAU TAY" };
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  Serial.println("Nong Trai Thong Minh - 2 Zone Hybrid");
  
  // Initialize button
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  
  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);  // Servo frequency: 50Hz
  Serial.println("PCA9685 initialized");
  
  // Initialize TFT
  tft.init();
  tft.setRotation(0); // Portrait (dọc)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  // Startup message
  tft.setCursor(10, 10);
  tft.println("Khoi dong he thong...");
  tft.println("Khu A: Khong khi");
  tft.println("Khu B: Dat");
  delay(2000);
  
  // Initialize Zone A (with PCA9685 for servo)
  zoneA = new ZoneA(&tft, &pwm);
  zoneA->begin();
  zoneA->setProfile(&PROFILE_LETTUCE);
  
  // Initialize Zone B (with PCA9685 for servos)
  zoneB = new ZoneB(&tft, &pwm);
  zoneB->begin();
  zoneB->setProfile(&PROFILE_LETTUCE);
  
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("He thong san sang!");
  Serial.println("Bam nut MODE de thay doi cay trong");
}

void loop() {
  // Check button for profile switching
  bool buttonState = digitalRead(PIN_BUTTON);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button pressed - switch profile for BOTH zones
    currentProfileIndex = (currentProfileIndex + 1) % 3;
    zoneA->setProfile(profiles[currentProfileIndex]);
    zoneB->setProfile(profiles[currentProfileIndex]);
    
    // Show profile change message
    tft.fillRect(0, 150, 240, 20, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.drawString("CAY: ", 10, 155, 1);
    tft.drawString(profileNames[currentProfileIndex], 50, 155, 1);
    
    Serial.print("Doi sang cay: ");
    Serial.println(profileNames[currentProfileIndex]);
    
    delay(1000); // Debounce and show message
    tft.fillRect(0, 150, 240, 20, TFT_BLACK); // Clear message
  }
  
  lastButtonState = buttonState;
  
  // Update Zone A at top (yOffset = 0)
  zoneA->update(0);
  
  // Update Zone B below Zone A (yOffset = 106)
  zoneB->update(106);
  
  delay(15); // Update at 10Hz
}
