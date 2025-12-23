#include <Arduino.h>
#include <TFT_eSPI.h>
#include "SmartZone.h"
#include "PlantProfile.h"

// MODE button
#define PIN_MODE_BUTTON 0 // Boot button

// Global objects
TFT_eSPI tft = TFT_eSPI();
PlantProfile currentProfile = PROFILE_LETTUCE;
ProfileThresholds thresholds;

// Three zones
SmartZone* zoneA = nullptr;
SmartZone* zoneB = nullptr;
SmartZone* zoneC = nullptr;

// Button state
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void switchProfile() {
  // Cycle through profiles
  switch(currentProfile) {
    case PROFILE_LETTUCE: currentProfile = PROFILE_MELON; break;
    case PROFILE_MELON: currentProfile = PROFILE_STRAWBERRY; break;
    case PROFILE_STRAWBERRY: currentProfile = PROFILE_LETTUCE; break;
  }
  
  loadProfileThresholds(currentProfile, thresholds);
  
  // Update all zones
  zoneA->updateProfile(&currentProfile, &thresholds);
  zoneB->updateProfile(&currentProfile, &thresholds);
  zoneC->updateProfile(&currentProfile, &thresholds);
  
  // Clear screen to redraw new profile name
  tft.fillScreen(TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  
  // Pass TFT to SmartZone class
  SmartZone::setTFT(&tft);
  
  // Setup MODE button
  pinMode(PIN_MODE_BUTTON, INPUT_PULLUP);
  
  // Load default profile
  loadProfileThresholds(currentProfile, thresholds);
  
  // Initialize Zones EARLY to reset Servos immediately
  zoneA = new SmartZone(1, ZONE_AIR, &currentProfile, &thresholds);
  zoneA->begin();
  
  zoneB = new SmartZone(2, ZONE_SOIL, &currentProfile, &thresholds);
  zoneB->begin();
  
  zoneC = new SmartZone(3, ZONE_HYDRO, &currentProfile, &thresholds);
  zoneC->begin();
  
  // Initialize TFT Display
  tft.init();
  tft.setRotation(0); // Portrait
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("SMART FARM SYSTEM");
  tft.println("Initializing...");
  delay(1000); // Now servos are already reset during this delay
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // Check MODE button
  bool buttonState = digitalRead(PIN_MODE_BUTTON);
  if (buttonState == LOW && lastButtonState == HIGH) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      switchProfile();
      lastDebounceTime = currentTime;
    }
  }
  lastButtonState = buttonState;
  
  // Run all zones
  if (zoneA) zoneA->run();
  if (zoneB) zoneB->run();
  if (zoneC) zoneC->run();
  
  delay(200); // UI Refresh rate
}