#include "ZoneA.h"

// ==================== SIMULATION CONSTANTS ====================
// Adjust these values to control simulation speed and behavior

// Temperature simulation rates (°C per cycle)
const float TEMP_COOLING_RATE = 0.15f;      // Cooling when fan is ON
const float TEMP_HEATING_RATE = 0.15f;      // Heating when heater is ON
const float TEMP_RECOVERY_RATE = 0.05f;     // Natural recovery to ambient

// Humidity simulation rates (% per cycle)
const float HUM_INCREASE_RATE = 0.3f;       // Humidifying when mist is ON
const float HUM_DECREASE_RATE = 0.1f;       // Natural drying

// Hysteresis values (prevent rapid on/off cycling)
const float TEMP_HYSTERESIS = 2.0f;         // Temperature dead band (°C)
const float HUM_HYSTERESIS = 5.0f;          // Humidity dead band (%)
const int LIGHT_HYSTERESIS = 10;            // Light level dead band (%)

// ============================================================


// Plant profile definitions
PlantProfile PROFILE_LETTUCE = {
  "XA LACH",
  28.0,  // tempMax - Fan ON if > 28°C
  15.0,  // tempMin - Heat ON if < 15°C  
  60,    // humidityMin - Mist ON if < 60%
  30,    // lightMin - Grow light ON if < 30%
  80,    // soilMoistureMin - Water ON if < 80%
  300,   // nMin - High N for leafy greens
  200,   // pMin
  200    // kMin
};

PlantProfile PROFILE_MELON = {
  "DUA LUOI",
  35.0,  // tempMax - Can withstand heat
  18.0,  // tempMin
  50,    // humidityMin - Prefers dry
  40,    // lightMin - Needs more light
  50,    // soilMoistureMin - Drought tolerant
  200,   // nMin
  250,   // pMin
  400    // kMin - High K for sweetness
};

PlantProfile PROFILE_STRAWBERRY = {
  "DAU TAY",
  24.0,  // tempMax - Cool loving
  12.0,  // tempMin
  65,    // humidityMin 
  35,    // lightMin
  65,    // soilMoistureMin
  250,   // nMin - Balanced
  250,   // pMin - Balanced
  300    // kMin
};

ZoneA::ZoneA(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver) {
  tft = display;
  dht = nullptr;
  // ringFan = nullptr;  // DISABLED - using servo via PCA9685
  ringHeat = nullptr;
  ringMist = nullptr;
  ringGrow = nullptr;
  pwm = pwmDriver;  // Shared PCA9685 instance
  
  // Pin assignments from diagram.json
  pinDHT = 26;
  pinLDR = 34;
  // pinRingFan = 13;  // Now servo on PCA9685 channel 4
  pinRingHeat = 12;
  pinRingMist = 14;
  pinRingGrow = 27;
  
  // Initialize states
  fanActive = false;
  heatActive = false;
  mistActive = false;
  growActive = false;
  
  simTempOffset = 0.0;
  simHumOffset = 0.0;
  
  prevRawTemp = 25.0;  // Initial guess
  prevRawHum = 50.0;
  
  // Default profile
  currentProfile = &PROFILE_LETTUCE;
}

ZoneA::~ZoneA() {
  if (dht) delete dht;
  // ringFan deleted - using servo
  if (ringHeat) delete ringHeat;
  if (ringMist) delete ringMist;
  if (ringGrow) delete ringGrow;
}

void ZoneA::begin() {
  // Initialize DHT22
  dht = new DHT(pinDHT, DHT22);
  dht->begin();
  
  // Initialize LED rings (12 pixels each) - Fan is servo
  // ringFan = new Adafruit_NeoPixel(12, pinRingFan, NEO_GRB + NEO_KHZ800);
  ringHeat = new Adafruit_NeoPixel(12, pinRingHeat, NEO_GRB + NEO_KHZ800);
  ringMist = new Adafruit_NeoPixel(12, pinRingMist, NEO_GRB + NEO_KHZ800);
  ringGrow = new Adafruit_NeoPixel(12, pinRingGrow, NEO_GRB + NEO_KHZ800);
  
  // ringFan->begin();
  ringHeat->begin();
  ringMist->begin();
  ringGrow->begin();
  
  // Turn off all rings
  // ringFan->clear();
  ringHeat->clear();
  ringMist->clear();
  ringGrow->clear();
  
  // ringFan->show();
  ringHeat->show();
  ringMist->show();
  ringGrow->show();
  
  // LDR is analog input
  pinMode(pinLDR, INPUT);
  
  // PCA9685 already initialized in main, just ensure servo is stopped
  pwm->setPWM(4, 0, 375);  // Channel 4 = Fan servo, 375 = stopped (90°)
}

void ZoneA::setProfile(PlantProfile* profile) {
  currentProfile = profile;
}

void ZoneA::updateSensors() {
  // Read DHT22
  float rawTemp = dht->readTemperature();
  float rawHum = dht->readHumidity();
  
  if (!isnan(rawTemp) && !isnan(rawHum)) {
    // Detect manual changes by comparing RAW values
    float tempDelta = abs(rawTemp - prevRawTemp);
    float humDelta = abs(rawHum - prevRawHum);
    
    if (tempDelta > 2.0) {
      // Large RAW change = user adjusted DHT22 → instant response
      simTempOffset = 0;
      temperature = rawTemp;
    } else {
      // Small/no RAW change = apply simulation physics
      temperature = rawTemp + simTempOffset;
    }
    
    // Always update previous raw value
    prevRawTemp = rawTemp;
    
    if (humDelta > 5.0) {
      // Large RAW change = user adjusted DHT22 → instant response
      simHumOffset = 0;
      humidity = rawHum;
    } else {
      // Small/no RAW change = apply simulation physics
      humidity = rawHum + simHumOffset;
    }
    
    // Always update previous raw value
    prevRawHum = rawHum;
  }
  
  // Read LDR - always instant (potentiometer)
  int ldrValue = analogRead(pinLDR);
  lightLevel = map(ldrValue, 0, 4095, 100, 0);  // Inverted: high reading = bright
  
  // Clamp values
  if (temperature < 0) temperature = 0;
  if (temperature > 60) temperature = 60;
  if (humidity < 0) humidity = 0;
  if (humidity > 100) humidity = 100;
  if (lightLevel < 0) lightLevel = 0;
  if (lightLevel > 100) lightLevel = 100;
}

void ZoneA::controlFan() {
  // Cooling fan servo - activates when too hot
  if (temperature > currentProfile->tempMax) {
    fanActive = true;
  } else if (temperature < currentProfile->tempMax - TEMP_HYSTERESIS) {
    fanActive = false;
  }
  
  // Control servo via PCA9685 (channel 4)
  if (fanActive) {
    // Sweep servo back and forth (0-180 degrees)
    servoFanPos += servoFanDirection * 15;  // Move 15 degrees per update (faster & smoother)
    
    // Reverse direction at limits
    if (servoFanPos >= 180) {
      servoFanPos = 180;
      servoFanDirection = -1;
    } else if (servoFanPos <= 0) {
      servoFanPos = 0;
      servoFanDirection = 1;
    }
    
    // Map position to PWM pulse (150=0deg, 600=180deg)
    int pulseWidth = map(servoFanPos, 0, 180, 150, 600);
    pwm->setPWM(4, 0, pulseWidth);
    
    // Simulate cooling effect (gradual)
    simTempOffset -= TEMP_COOLING_RATE;
  } else {
    // Servo stopped at center
    servoFanPos = 90;
    pwm->setPWM(4, 0, 375);  // Mid position
    
    // Natural temperature recovery
    if (simTempOffset < -0.1) {
      simTempOffset += TEMP_RECOVERY_RATE;
    } else if (simTempOffset < 0) {
      simTempOffset = 0;
    }
  }
}

void ZoneA::controlHeat() {
  // Heater - activates when too cold
  if (temperature < currentProfile->tempMin) {
    heatActive = true;
  } else if (temperature > currentProfile->tempMin + TEMP_HYSTERESIS) {
    heatActive = false; // Hysteresis to prevent rapid cycling
  }
  
  // Update LED ring
  if (heatActive) {
    for (int i = 0; i < ringHeat->numPixels(); i++) {
      ringHeat->setPixelColor(i, ringHeat->Color(255, 0, 0)); // Red
    }
    // Simulate heating effect (gradual)
    simTempOffset += TEMP_HEATING_RATE;
  } else {
    ringHeat->clear();
    // Natural temperature recovery
    if (simTempOffset > 0.1) {
      simTempOffset -= TEMP_RECOVERY_RATE;
    } else if (simTempOffset > 0) {
      simTempOffset = 0;
    }
  }
  ringHeat->show();
}

void ZoneA::controlMist() {
  // Humidifier/Mist - activates when too dry
  if (humidity < currentProfile->humidityMin) {
    mistActive = true;
  } else if (humidity > currentProfile->humidityMin + HUM_HYSTERESIS) {
    mistActive = false; // Hysteresis to prevent rapid cycling
  }
  
  // Update LED ring
  if (mistActive) {
    for (int i = 0; i < ringMist->numPixels(); i++) {
      ringMist->setPixelColor(i, ringMist->Color(0, 255, 255)); // Cyan
    }
    // Simulate humidifying effect (gradual)
    simHumOffset += HUM_INCREASE_RATE;
  } else {
    ringMist->clear();
    // Natural humidity decrease
    if (simHumOffset > 0.1) {
      simHumOffset -= HUM_DECREASE_RATE;
    } else if (simHumOffset > 0) {
      simHumOffset = 0;
    }
  }
  ringMist->show();
}

void ZoneA::controlGrowLight() {
  // Grow light - activates when too dark
  if (lightLevel < currentProfile->lightMin) {
    growActive = true;
  } else if (lightLevel > currentProfile->lightMin + LIGHT_HYSTERESIS) {
    growActive = false; // Hysteresis to prevent rapid cycling
  }
  
  // Update LED ring
  if (growActive) {
    // Purple/Magenta for grow light
    for (int i = 0; i < ringGrow->numPixels(); i++) {
      ringGrow->setPixelColor(i, ringGrow->Color(255, 0, 255)); // Magenta
    }
  } else {
    ringGrow->clear();
  }
  
  ringGrow->show();
}

void ZoneA::updateDisplay(int yOffset) {
  if (!tft) return;
  
  // Draw zone header
  tft->fillRect(0, yOffset, 240, 20, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(1);
  tft->drawString("KHU A: KHONG KHI", 10, yOffset + 5);
  tft->drawString(currentProfile->name, 180, yOffset + 5);
  
  int yBase = yOffset + 25;
  
  // Display sensor readings
  tft->setTextColor(TFT_GREEN, TFT_BLACK);
  char buffer[32];
  
  sprintf(buffer, "Nhiet do: %.1f C", temperature);
  tft->drawString(buffer, 10, yBase);
  
  sprintf(buffer, "Do am: %.0f %%", humidity);
  tft->drawString(buffer, 10, yBase + 20);
  
  sprintf(buffer, "Anh sang: %d %%", lightLevel);
  tft->drawString(buffer, 10, yBase + 40);
  
  // Display actuator states
  int statusY = yBase + 65;
  tft->setTextFont(1);
  
  tft->setTextColor(fanActive ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("QUAT", 10, statusY);
  
  tft->setTextColor(heatActive ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("SUOI", 60, statusY);
  
  tft->setTextColor(mistActive ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("SUONG", 110, statusY);
  
  tft->setTextColor(growActive ? TFT_MAGENTA : TFT_DARKGREY, TFT_BLACK);
  tft->drawString("DEN", 170, statusY);
}

void ZoneA::update(int displayYOffset) {
  updateSensors();
  controlFan();
  controlHeat();
  controlMist();
  controlGrowLight();
  updateDisplay(displayYOffset);
}
