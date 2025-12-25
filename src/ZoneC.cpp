#include "ZoneC.h"
#include "ZoneA.h"  // For PlantProfile

// ==================== SIMULATION CONSTANTS ====================

// Water temperature simulation rates (°C per cycle)
const float TEMP_COOLING_RATE = 0.3f;    // Chiller cooling rate
const float TEMP_WARMING_RATE = 0.1f;    // Natural warming

// EC simulation rates (mS/cm per cycle)
const float EC_INCREASE_RATE = 0.05f;    // Adding nutrients
const float EC_DECREASE_RATE = 0.02f;    // Plant consumption

// Hysteresis values
const float TEMP_HYSTERESIS = 1.0f;      // Temperature dead band (°C)
const float EC_HYSTERESIS = 0.2f;        // EC dead band (mS/cm)

// Target ranges (simplified - can be per-profile later)
const float TARGET_TEMP_MAX = 25.0f;     // Max water temp (°C)
const float TARGET_EC_MIN = 1.2f;        // Min EC (mS/cm)

// ============================================================

ZoneC::ZoneC(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver) {
  tft = display;
  pwm = pwmDriver;
  
  // Pin assignments - only EC and pH (no temp)
  pinPotEC = 4;      // GPIO4 (ADC2_CH0) - EC pot
  pinPotPH = 33;     // GPIO33 (ADC1_CH5) - pH pot
  
  // Initialize states
  nutrientActive = false;  // Only nutrient pump (no chiller)
  
  simECOffset = 0.0;
  prevRawEC = 1500;
  
  // Initialize sensor readings
  hydroEC = 1.5;
  hydroPH = 6.5;
  
  currentProfile = &PROFILE_LETTUCE;
}

ZoneC::~ZoneC() {
  // No DHT to delete
}

void ZoneC::begin() {
  // Stop servo initially (center position)
  pwm->setPWM(6, 0, 375);  // Nutrient servo only
}

void ZoneC::setProfile(PlantProfile* profile) {
  currentProfile = profile;
}

void ZoneC::updateSensors() {
  // Read EC (Electrical Conductivity) 0-3.0 mS/cm range - ALWAYS reads
  int rawEC = analogRead(pinPotEC);
  float baseEC = map(rawEC, 0, 4095, 0, 300) / 100.0;  // 0.00-3.00 mS/cm
  
  // Detect manual changes
  int ecDelta = abs(rawEC - prevRawEC);
  if (ecDelta > 200) {  // ~0.15 mS/cm threshold
    simECOffset = 0;
    hydroEC = baseEC;
  } else {
    hydroEC = baseEC + simECOffset;
  }
  prevRawEC = rawEC;
  
  // Clamp EC
  if (hydroEC < 0) hydroEC = 0;
  if (hydroEC > 3.0) hydroEC = 3.0;
  
  // Read pH (0-14, instant - always direct reading)
  int rawPH = analogRead(pinPotPH);
  hydroPH = map(rawPH, 0, 4095, 0, 140) / 10.0;
}

// No chiller - removed

void ZoneC::controlNutrient() {
  // Nutrient pump activates when EC too low
  if (hydroEC < TARGET_EC_MIN) {
    nutrientActive = true;
  } else if (hydroEC > TARGET_EC_MIN + EC_HYSTERESIS) {
    nutrientActive = false;
  }
  
  // Control servo via PCA9685 (channel 6)
  if (nutrientActive) {
    // Sweep servo for nutrient pump
    servoNutrientPos += servoNutrientDir * 15;
    if (servoNutrientPos >= 180) {
      servoNutrientPos = 180;
      servoNutrientDir = -1;
    } else if (servoNutrientPos <= 0) {
      servoNutrientPos = 0;
      servoNutrientDir = 1;
    }
    pwm->setPWM(6, 0, map(servoNutrientPos, 0, 180, 150, 600));
    
    // Simulate EC increase
    simECOffset += EC_INCREASE_RATE;
  } else {
    // Servo stopped
    servoNutrientPos = 90;
    pwm->setPWM(6, 0, 375);
    
    // Natural EC decrease (plant consumption)
    if (simECOffset > 0.01) {
      simECOffset -= EC_DECREASE_RATE;
    } else if (simECOffset > 0) {
      simECOffset = 0;
    }
  }
}

void ZoneC::updateDisplay(int yOffset) {
  if (!tft) return;
  
  // Draw zone header
  tft->fillRect(0, yOffset, 240, 20, TFT_DARKGREEN);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft->setTextFont(1);
  tft->drawString("KHU C: HYDRO", 10, yOffset + 5);
  tft->drawString(currentProfile->name, 180, yOffset + 5);
  
  int yBase = yOffset + 25;
  
  // Display sensor readings
  char buffer[32];
  
  tft->setTextColor(TFT_CYAN, TFT_BLACK);
  sprintf(buffer, "EC: %.2f mS", hydroEC);
  tft->drawString(buffer, 10, yBase);
  
  // pH display with color coding based on safety levels (same as Zone B)
  uint16_t phColor;
  if (hydroPH < 5.0 || hydroPH > 8.0) {
    phColor = TFT_RED;      // Nguy hiem
  } else if (hydroPH < 5.5 || hydroPH > 7.5) {
    phColor = TFT_YELLOW;   // Canh bao
  } else {
    phColor = TFT_GREEN;    // An toan
  }
  
  tft->setTextColor(phColor, TFT_BLACK);
  sprintf(buffer, "pH: %.1f", hydroPH);
  tft->drawString(buffer, 120, yBase);
  
  
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  if (nutrientActive) {
    tft->drawString("Pump: ON ", 10, yBase + 20);
  } else {
    tft->drawString("Pump: OFF", 10, yBase + 20);
  }
}

void ZoneC::update(int displayYOffset) {
  updateSensors();
  controlNutrient();  // Only nutrient pump
  updateDisplay(displayYOffset);
}
