#include "ZoneB.h"
#include "ZoneA.h"  // For PlantProfile

// ==================== SIMULATION CONSTANTS ====================
// Adjust these values to control simulation speed and behavior

// Soil moisture simulation rates (% per cycle)
const float MOIST_INCREASE_RATE = 0.5f;    // Watering rate
const float MOIST_DECREASE_RATE = 0.1f;    // Natural drying

// Nutrient simulation rates (mg/kg per cycle)
const float NUTRIENT_INCREASE_RATE = 0.8f; // Adding nutrients
const float NUTRIENT_DECREASE_RATE = 0.2f; // Plant consumption

// Hysteresis values
const int MOIST_HYSTERESIS = 10;           // Moisture dead band (%)
const int NUTRIENT_HYSTERESIS = 30;        // Nutrient dead band (mg/kg)

// pH alarm thresholds (from detai.md)
const float PH_MIN_ALARM = 5.0f;
const float PH_MAX_ALARM = 8.0f;

// ============================================================

ZoneB::ZoneB(TFT_eSPI* display) {
  tft = display;
  ringWater = nullptr;
  ringN = nullptr;
  ringP = nullptr;
  ringK = nullptr;
  
  // Pin assignments from diagram.json
  pinPotMoist = 36;  // VP
  pinPotPH = 32;
  pinPotN = 39;      // VN
  pinPotP = 35;
  pinPotK = 25;
  pinRingWater = 16; // RX2
  pinRingN = 17;     // TX2
  pinRingP = 5;
  pinRingK = 1;      // TX0
  pinBuzzer = 3;     // RX0
  
  // Initialize states
  waterActive = false;
  nActive = false;
  pActive = false;
  kActive = false;
  alarmActive = false;
  
  simMoistOffset = 0.0;
  simNOffset = 0.0;
  simPOffset = 0.0;
  simKOffset = 0.0;
  
  currentProfile = &PROFILE_LETTUCE;
}

ZoneB::~ZoneB() {
  if (ringWater) delete ringWater;
  if (ringN) delete ringN;
  if (ringP) delete ringP;
  if (ringK) delete ringK;
}

void ZoneB::begin() {
  // Initialize LED rings (12 pixels each)
  ringWater = new Adafruit_NeoPixel(12, pinRingWater, NEO_GRB + NEO_KHZ800);
  ringN = new Adafruit_NeoPixel(12, pinRingN, NEO_GRB + NEO_KHZ800);
  ringP = new Adafruit_NeoPixel(12, pinRingP, NEO_GRB + NEO_KHZ800);
  ringK = new Adafruit_NeoPixel(12, pinRingK, NEO_GRB + NEO_KHZ800);
  
  ringWater->begin();
  ringN->begin();
  ringP->begin();
  ringK->begin();
  
  // Turn off all rings
  ringWater->clear();
  ringN->clear();
  ringP->clear();
  ringK->clear();
  
  ringWater->show();
  ringN->show();
  ringP->show();
  ringK->show();
  
  // Buzzer output
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, LOW);
}

void ZoneB::setProfile(PlantProfile* profile) {
  currentProfile = profile;
}

void ZoneB::updateSensors() {
  // Read soil moisture (0-100%)
  int rawMoist = analogRead(pinPotMoist);
  int baseMoist = map(rawMoist, 0, 4095, 0, 100);
  soilMoisture = baseMoist + simMoistOffset;
  if (soilMoisture > 100) soilMoisture = 100;
  if (soilMoisture < 0) soilMoisture = 0;
  
  // Read soil pH (0-14, mapped from 0-140 for 0.1 precision)
  int rawPH = analogRead(pinPotPH);
  soilPH = map(rawPH, 0, 4095, 0, 140) / 10.0;
  
  // Read NPK levels (0-500 mg/kg range)
  int rawN = analogRead(pinPotN);
  int baseN = map(rawN, 0, 4095, 0, 500);
  nitrogenLevel = baseN + simNOffset;
  if (nitrogenLevel < 0) nitrogenLevel = 0;
  
  int rawP = analogRead(pinPotP);
  int baseP = map(rawP, 0, 4095, 0, 500);
  phosphorusLevel = baseP + simPOffset;
  if (phosphorusLevel < 0) phosphorusLevel = 0;
  
  int rawK = analogRead(pinPotK);
  int baseK = map(rawK, 0, 4095, 0, 500);
  potassiumLevel = baseK + simKOffset;
  if (potassiumLevel < 0) potassiumLevel = 0;
}

void ZoneB::checkAlarm() {
  // Check pH range (from detai.md - safety check)
  if (soilPH < PH_MIN_ALARM || soilPH > PH_MAX_ALARM) {
    if (!alarmActive) {
      alarmActive = true;
      tone(pinBuzzer, 1000); // Sound alarm
    }
    
    // LOCK all nutrient pumps for safety
    nActive = false;
    pActive = false;
    kActive = false;
    
    // Update LED rings
    ringN->clear();
    ringP->clear();
    ringK->clear();
    ringN->show();
    ringP->show();
    ringK->show();
    
  } else {
    if (alarmActive) {
      alarmActive = false;
      noTone(pinBuzzer);
    }
  }
}

void ZoneB::controlWater() {
  // Water control - with hysteresis
  if (soilMoisture < currentProfile->soilMoistureMin) {
    waterActive = true;
  } else if (soilMoisture > currentProfile->soilMoistureMin + MOIST_HYSTERESIS) {
    waterActive = false;
  }
  
  // Update LED ring
  if (waterActive) {
    // Green color for water
    for (int i = 0; i < ringWater->numPixels(); i++) {
      ringWater->setPixelColor(i, ringWater->Color(0, 255, 0)); // Green
    }
    
    // Simulate watering
    simMoistOffset += MOIST_INCREASE_RATE;
  } else {
    ringWater->clear();
    
    // Natural drying
    if (simMoistOffset > 0.1) {
      simMoistOffset -= MOIST_DECREASE_RATE;
    } else if (simMoistOffset > 0) {
      simMoistOffset = 0;
    }
  }
  
  ringWater->show();
}

void ZoneB::controlNutrients() {
  // Skip if alarm active (safety lockout from detai.md)
  if (alarmActive) return;
  
  // Nitrogen control
  if (nitrogenLevel < currentProfile->nMin) {
    nActive = true;
  } else if (nitrogenLevel > currentProfile->nMin + NUTRIENT_HYSTERESIS) {
    nActive = false;
  }
  
  if (nActive) {
    // Red color for Nitrogen
    for (int i = 0; i < ringN->numPixels(); i++) {
      ringN->setPixelColor(i, ringN->Color(255, 0, 0)); // Red
    }
    simNOffset += NUTRIENT_INCREASE_RATE;
  } else {
    ringN->clear();
    if (simNOffset > 0.1) simNOffset -= NUTRIENT_DECREASE_RATE;
    else if (simNOffset > 0) simNOffset = 0;
  }
  ringN->show();
  
  // Phosphorus control
  if (phosphorusLevel < currentProfile->pMin) {
    pActive = true;
  } else if (phosphorusLevel > currentProfile->pMin + NUTRIENT_HYSTERESIS) {
    pActive = false;
  }
  
  if (pActive) {
    // Yellow color for Phosphorus
    for (int i = 0; i < ringP->numPixels(); i++) {
      ringP->setPixelColor(i, ringP->Color(255, 255, 0)); // Yellow
    }
    simPOffset += NUTRIENT_INCREASE_RATE;
  } else {
    ringP->clear();
    if (simPOffset > 0.1) simPOffset -= NUTRIENT_DECREASE_RATE;
    else if (simPOffset > 0) simPOffset = 0;
  }
  ringP->show();
  
  // Potassium control
  if (potassiumLevel < currentProfile->kMin) {
    kActive = true;
  } else if (potassiumLevel > currentProfile->kMin + NUTRIENT_HYSTERESIS) {
    kActive = false;
  }
  
  if (kActive) {
    // Orange color for Potassium
    for (int i = 0; i < ringK->numPixels(); i++) {
      ringK->setPixelColor(i, ringK->Color(255, 128, 0)); // Orange
    }
    simKOffset += NUTRIENT_INCREASE_RATE;
  } else {
    ringK->clear();
    if (simKOffset > 0.1) simKOffset -= NUTRIENT_DECREASE_RATE;
    else if (simKOffset > 0) simKOffset = 0;
  }
  ringK->show();
}

void ZoneB::updateDisplay(int yOffset) {
  if (!tft) return;
  
  // Draw zone header
  tft->fillRect(0, yOffset, 240, 20, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(1);
  tft->drawString("KHU B: DAT", 10, yOffset + 5);
  tft->drawString(currentProfile->name, 180, yOffset + 5);
  
  int yBase = yOffset + 25;
  
  // Display sensor readings
  char buffer[32];
  
  tft->setTextColor(TFT_BLUE, TFT_BLACK);
  sprintf(buffer, "Do am: %d %%", soilMoisture);
  tft->drawString(buffer, 10, yBase);
  
  // pH display with color coding based on safety levels
  uint16_t phColor;
  if (soilPH < 5.0 || soilPH > 8.0) {
    phColor = TFT_RED;      // Nguy hiểm
  } else if (soilPH < 5.5 || soilPH > 7.5) {
    phColor = TFT_YELLOW;   // Cảnh báo
  } else {
    phColor = TFT_GREEN;    // An toàn
  }
  
  tft->setTextColor(phColor, TFT_BLACK);
  sprintf(buffer, "pH: %.1f", soilPH);
  tft->drawString(buffer, 10, yBase + 20, 1);
  
  // NPK levels
  tft->setTextColor(TFT_PURPLE, TFT_BLACK);
  sprintf(buffer, "N:%dmg", nitrogenLevel);
  tft->drawString(buffer, 10, yBase + 40, 1);
  
  sprintf(buffer, "P:%dmg", phosphorusLevel);
  tft->drawString(buffer, 80, yBase + 40, 1);
  
  sprintf(buffer, "K:%dmg", potassiumLevel);
  tft->drawString(buffer, 150, yBase + 40, 1);
  
  
  // Display actuator states (always shown, alarm overlay removed)
  int statusY = yBase + 65;
  tft->setTextFont(1);
  
  tft->setTextColor(waterActive ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("NUOC", 10, statusY, 1);
  
  tft->setTextColor(nActive ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("N", 60, statusY, 1);
  
  tft->setTextColor(pActive ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("P", 100, statusY, 1);
  
  tft->setTextColor(kActive ? TFT_ORANGE : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("K", 140, statusY, 1);
}

void ZoneB::update(int displayYOffset) {
  updateSensors();
  checkAlarm();
  controlWater();
  controlNutrients();
  
  // MANUAL OVERRIDE - only reset when pot low AND device NOT running
  // This allows simulation to work when pumps are active
  int rawMoist = analogRead(pinPotMoist);
  int baseMoist = map(rawMoist, 0, 4095, 0, 100);
  if (baseMoist < 10 && !waterActive) {
    simMoistOffset = 0;  // Reset only if water pump is OFF
  }
  
  int rawN = analogRead(pinPotN);
  int baseN = map(rawN, 0, 4095, 0, 500);
  if (baseN < 30 && !nActive) {
    simNOffset = 0;  // Reset only if N pump is OFF
  }
  
  int rawP = analogRead(pinPotP);
  int baseP = map(rawP, 0, 4095, 0, 500);
  if (baseP < 30 && !pActive) {
    simPOffset = 0;  // Reset only if P pump is OFF
  }
  
  int rawK = analogRead(pinPotK);
  int baseK = map(rawK, 0, 4095, 0, 500);
  if (baseK < 30 && !kActive) {
    simKOffset = 0;  // Reset only if K pump is OFF
  }
  
  updateDisplay(displayYOffset);
}
