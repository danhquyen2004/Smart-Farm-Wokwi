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

ZoneA::ZoneA(TFT_eSPI* display) {
  tft = display;
  dht = nullptr;
  ringFan = nullptr;
  ringHeat = nullptr;
  ringMist = nullptr;
  ringGrow = nullptr;
  
  // Pin assignments from diagram.json
  pinDHT = 26;
  pinLDR = 34;
  pinRingFan = 13;
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
  
  // Default profile
  currentProfile = &PROFILE_LETTUCE;
}

ZoneA::~ZoneA() {
  if (dht) delete dht;
  if (ringFan) delete ringFan;
  if (ringHeat) delete ringHeat;
  if (ringMist) delete ringMist;
  if (ringGrow) delete ringGrow;
}

void ZoneA::begin() {
  // Initialize DHT22
  dht = new DHT(pinDHT, DHT22);
  dht->begin();
  
  // Initialize LED rings (12 pixels each)
  ringFan = new Adafruit_NeoPixel(12, pinRingFan, NEO_GRB + NEO_KHZ800);
  ringHeat = new Adafruit_NeoPixel(12, pinRingHeat, NEO_GRB + NEO_KHZ800);
  ringMist = new Adafruit_NeoPixel(12, pinRingMist, NEO_GRB + NEO_KHZ800);
  ringGrow = new Adafruit_NeoPixel(12, pinRingGrow, NEO_GRB + NEO_KHZ800);
  
  ringFan->begin();
  ringHeat->begin();
  ringMist->begin();
  ringGrow->begin();
  
  // Turn off all rings
  ringFan->clear();
  ringHeat->clear();
  ringMist->clear();
  ringGrow->clear();
  
  ringFan->show();
  ringHeat->show();
  ringMist->show();
  ringGrow->show();
  
  // LDR is analog input
  pinMode(pinLDR, INPUT);
}

void ZoneA::setProfile(PlantProfile* profile) {
  currentProfile = profile;
}

void ZoneA::updateSensors() {
  // Read temperature and humidity
  float rawTemp = dht->readTemperature();
  float rawHum = dht->readHumidity();
  
  // Apply simulation offsets for visual feedback
  if (!isnan(rawTemp)) {
    temperature = rawTemp + simTempOffset;
  }
  
  if (!isnan(rawHum)) {
    humidity = rawHum + simHumOffset;
    if (humidity > 100) humidity = 100;
    if (humidity < 0) humidity = 0;
  }
  
  // Read light level (map to 0-100%)
  int rawLDR = analogRead(pinLDR);
  lightLevel = map(rawLDR, 0, 4095, 100, 0); // Invert: dark=0, bright=100
}

void ZoneA::controlFan() {
  // Cooling fan - activates when too hot
  if (temperature > currentProfile->tempMax) {
    fanActive = true;
  } else if (temperature < currentProfile->tempMax - TEMP_HYSTERESIS) {
    fanActive = false; // Hysteresis to prevent rapid cycling
  }
  
  // Update LED ring
  if (fanActive) {
    // Cyan color for cooling fan
    for (int i = 0; i < ringFan->numPixels(); i++) {
      ringFan->setPixelColor(i, ringFan->Color(0, 255, 255)); // Cyan
    }
    
    // Simulate cooling effect
    simTempOffset -= TEMP_COOLING_RATE;
  } else {
    ringFan->clear();
    
    // Natural temperature recovery
    if (simTempOffset < -0.1) {
      simTempOffset += TEMP_RECOVERY_RATE;
    } else if (simTempOffset < 0) {
      simTempOffset = 0;
    }
  }
  
  ringFan->show();
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
    // Red color for heater
    for (int i = 0; i < ringHeat->numPixels(); i++) {
      ringHeat->setPixelColor(i, ringHeat->Color(255, 0, 0)); // Red
    }
    
    // Simulate heating effect
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
    // Cyan color for mist
    for (int i = 0; i < ringMist->numPixels(); i++) {
      ringMist->setPixelColor(i, ringMist->Color(0, 255, 255)); // Cyan
    }
    
    // Simulate humidifying effect
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
