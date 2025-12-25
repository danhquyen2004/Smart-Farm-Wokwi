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

ZoneB::ZoneB(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver) {
  tft = display;
  pwm = pwmDriver;
  
  // Pin assignments from diagram.json
  pinPotMoist = 36;  // VP
  pinPotPH = 32;
  pinPotN = 39;      // VN
  pinPotP = 35;
  pinPotK = 25;
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
  
  prevRawMoist = 50;
  prevRawN = 300;
  prevRawP = 200;
  prevRawK = 250;
  
  currentProfile = &PROFILE_LETTUCE;
}

ZoneB::~ZoneB() {
  // No LED rings to delete - servos controlled via PCA9685
}

void ZoneB::begin() {
  // No LED rings - using servos via PCA9685
  // Servos already initialized in main.cpp
  
  // Buzzer output
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, LOW);
  
  // Stop all servos initially (center position)
  pwm->setPWM(0, 0, 375);  // Water servo
  pwm->setPWM(1, 0, 375);  // N servo
  pwm->setPWM(2, 0, 375);  // P servo
  pwm->setPWM(3, 0, 375);  // K servo
}

void ZoneB::setProfile(PlantProfile* profile) {
  currentProfile = profile;
}

void ZoneB::updateSensors() {
  // Read soil moisture (0-100%)
  int rawMoist = analogRead(pinPotMoist);
  int baseMoist = map(rawMoist, 0, 4095, 0, 100);
  
  // Detect manual changes
  int moistDelta = abs(rawMoist - prevRawMoist);
  if (moistDelta > 200) {  // ~5% threshold
    simMoistOffset = 0;
    soilMoisture = baseMoist;
  } else {
    soilMoisture = baseMoist + simMoistOffset;
  }
  prevRawMoist = rawMoist;
  
  // Clamp
  if (soilMoisture > 100) soilMoisture = 100;
  if (soilMoisture < 0) soilMoisture = 0;
  
  // Read soil pH (0-14, instant - always direct reading)
  int rawPH = analogRead(pinPotPH);
  soilPH = map(rawPH, 0, 4095, 0, 140) / 10.0;
  
  // Read NPK levels (0-500 mg/kg) with instant detection
  int rawN = analogRead(pinPotN);
  int baseN = map(rawN, 0, 4095, 0, 500);
  int nDelta = abs(rawN - prevRawN);
  if (nDelta > 100) {  // ~12mg/kg threshold
    simNOffset = 0;
    nitrogenLevel = baseN;
  } else {
    nitrogenLevel = baseN + simNOffset;
  }
  prevRawN = rawN;
  if (nitrogenLevel < 0) nitrogenLevel = 0;
  
  int rawP = analogRead(pinPotP);
  int baseP = map(rawP, 0, 4095, 0, 500);
  int pDelta = abs(rawP - prevRawP);
  if (pDelta > 100) {
    simPOffset = 0;
    phosphorusLevel = baseP;
  } else {
    phosphorusLevel = baseP + simPOffset;
  }
  prevRawP = rawP;
  if (phosphorusLevel < 0) phosphorusLevel = 0;
  
  int rawK = analogRead(pinPotK);
  int baseK = map(rawK, 0, 4095, 0, 500);
  int kDelta = abs(rawK - prevRawK);
  if (kDelta > 100) {
    simKOffset = 0;
    potassiumLevel = baseK;
  } else {
    potassiumLevel = baseK + simKOffset;
  }
  prevRawK = rawK;
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
    
    // Stop all nutrient servos
    pwm->setPWM(1, 0, 375);  // N servo center
    pwm->setPWM(2, 0, 375);  // P servo center
    pwm->setPWM(3, 0, 375);  // K servo center
    
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
  
  // Control servo via PCA9685 (channel 0)
  if (waterActive) {
    // Sweep servo for water pump
    servoWaterPos += servoWaterDir * 15;
    if (servoWaterPos >= 180) {
      servoWaterPos = 180;
      servoWaterDir = -1;
    } else if (servoWaterPos <= 0) {
      servoWaterPos = 0;
      servoWaterDir = 1;
    }
    pwm->setPWM(0, 0, map(servoWaterPos, 0, 180, 150, 600));
    
    // Simulate watering
    simMoistOffset += MOIST_INCREASE_RATE;
  } else {
    // Servo stopped
    servoWaterPos = 90;
    pwm->setPWM(0, 0, 375);
    
    // Natural drying
    if (simMoistOffset > 0.1) {
      simMoistOffset -= MOIST_DECREASE_RATE;
    } else if (simMoistOffset > 0) {
      simMoistOffset = 0;
    }
  }
}

void ZoneB::controlNutrients() {
  // Skip if alarm active (safety lockout from detai.md)
  if (alarmActive) return;
  
  // Nitrogen control (PCA9685 channel 1)
  if (nitrogenLevel < currentProfile->nMin) {
    nActive = true;
  } else if (nitrogenLevel > currentProfile->nMin + NUTRIENT_HYSTERESIS) {
    nActive = false;
  }
  
  if (nActive) {
    servoNPos += servoNDir * 15;
    if (servoNPos >= 180) {
      servoNPos = 180;
      servoNDir = -1;
    } else if (servoNPos <= 0) {
      servoNPos = 0;
      servoNDir = 1;
    }
    pwm->setPWM(1, 0, map(servoNPos, 0, 180, 150, 600));
    simNOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoNPos = 90;
    pwm->setPWM(1, 0, 375);
    if (simNOffset > 0.1) simNOffset -= NUTRIENT_DECREASE_RATE;
    else if (simNOffset > 0) simNOffset = 0;
  }
  
  // Phosphorus control (PCA9685 channel 2)
  if (phosphorusLevel < currentProfile->pMin) {
    pActive = true;
  } else if (phosphorusLevel > currentProfile->pMin + NUTRIENT_HYSTERESIS) {
    pActive = false;
  }
  
  if (pActive) {
    servoPPos += servoPDir * 15;
    if (servoPPos >= 180) {
      servoPPos = 180;
      servoPDir = -1;
    } else if (servoPPos <= 0) {
      servoPPos = 0;
      servoPDir = 1;
    }
    pwm->setPWM(2, 0, map(servoPPos, 0, 180, 150, 600));
    simPOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoPPos = 90;
    pwm->setPWM(2, 0, 375);
    if (simPOffset > 0.1) simPOffset -= NUTRIENT_DECREASE_RATE;
    else if (simPOffset > 0) simPOffset = 0;
  }
  
  // Potassium control (PCA9685 channel 3)
  if (potassiumLevel < currentProfile->kMin) {
    kActive = true;
  } else if (potassiumLevel > currentProfile->kMin + NUTRIENT_HYSTERESIS) {
    kActive = false;
  }
  
  if (kActive) {
    servoKPos += servoKDir * 15;
    if (servoKPos >= 180) {
      servoKPos = 180;
      servoKDir = -1;
    } else if (servoKPos <= 0) {
      servoKPos = 0;
      servoKDir = 1;
    }
    pwm->setPWM(3, 0, map(servoKPos, 0, 180, 150, 600));
    simKOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoKPos = 90;
    pwm->setPWM(3, 0, 375);
    if (simKOffset > 0.1) simKOffset -= NUTRIENT_DECREASE_RATE;
    else if (simKOffset > 0) simKOffset = 0;
  }
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
