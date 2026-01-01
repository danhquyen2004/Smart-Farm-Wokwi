#include "PlantZone.h"

// ==================== SIMULATION CONSTANTS ====================
// From simulation_logic.md backup

// Temperature rates (°C per cycle)
const float TEMP_COOLING_RATE = 0.15f;
const float TEMP_HEATING_RATE = 0.15f;
const float TEMP_RECOVERY_RATE = 0.05f;

// Humidity rates (% per cycle)
const float HUM_INCREASE_RATE = 0.3f;
const float HUM_DECREASE_RATE = 0.1f;

// Moisture rates (% per cycle)
const float MOIST_INCREASE_RATE = 0.5f;
const float MOIST_DECREASE_RATE = 0.1f;

// Nutrient rates (mg/kg per cycle)
const float NUTRIENT_INCREASE_RATE = 0.8f;
const float NUTRIENT_DECREASE_RATE = 0.2f;

// EC rates (mS/cm per cycle)
const float EC_INCREASE_RATE = 0.05f;
const float EC_DECREASE_RATE = 0.01f;

// Hysteresis values
const float TEMP_HYSTERESIS = 2.0f;
const float HUM_HYSTERESIS = 5.0f;
const int LIGHT_HYSTERESIS = 10;
const int MOIST_HYSTERESIS = 10;
const int NUTRIENT_HYSTERESIS = 30;
const float EC_HYSTERESIS = 0.2f;

// ==================== PLANT PROFILES ====================

PlantProfile PROFILE_LETTUCE = {
  "XA LACH",
  28.0f, 15.0f, 60, 30,           // Air: tempMax, tempMin, humMin, lightMin
  80, 5.5f, 7.0f, 300, 200, 200,  // Soil: moistMin, phMin, phMax, N, P, K
  1.2f, 2.0f                       // Hydro: ecMin, ecMax
};

PlantProfile PROFILE_STRAWBERRY = {
  "DAU TAY",
  24.0f, 12.0f, 65, 35,
  65, 5.5f, 6.5f, 250, 250, 300,
  1.0f, 1.8f
};

PlantProfile PROFILE_TOMATO = {
  "CA CHUA",
  30.0f, 18.0f, 55, 40,
  70, 6.0f, 7.0f, 200, 250, 350,
  2.0f, 3.0f
};

// ==================== STATIC MEMBERS ====================

// Shared DHT sensor across all zones
DHT* PlantZone::sharedDHT = nullptr;

// ==================== CONSTRUCTOR / DESTRUCTOR ====================

PlantZone::PlantZone(uint8_t id, TFT_eSPI* display, Adafruit_PWMServoDriver* servo, MuxManager* multiplexer) {
  zoneId = id;
  tft = display;
  pwm = servo;
  mux = multiplexer;
  
  dht = nullptr;
  ringHeat = nullptr;
  ringMist = nullptr;
  ringGrow = nullptr;
  
  // ESP32 DevKit V1 - Shared Environment Edition
  // All zones share the same environment sensors
  
  // Shared sensor pins (only Zone 0 initializes, Zone 1 reads same values)
  pinDHT = 25;      // Shared DHT22
  pinLDR = 36;      // Shared LDR (VP)
  
  // Zone-specific actuator pins
  if (zoneId == 0) {
    pinBuzzer = 33;
    pinRingHeat = 12;
  } else {
    pinBuzzer = 32;
    pinRingHeat = 13;
  }
  
  
  // PCA9685 channels (Zone 1: 0-5, Zone 2: 6-11)
  pwmChFan = zoneId * 6;
  pwmChWater = zoneId * 6 + 1;
  pwmChNutrient = zoneId * 6 + 5;  // Channel 5 or 11 for nutrient
  
  // Initialize sensor values
  temperature = 25.0f;
  humidity = 50.0f;
  lightLevel = 50;
  moisture = 50;
  soilPH = 6.5f;
  nitrogen = 300;
  phosphorus = 200;
  potassium = 250;
  hydroEC = 1.5f;
  
  // Initialize actuator states
  fanActive = false;
  heatActive = false;
  mistActive = false;
  growActive = false;
  waterActive = false;
  nutrientActive = false;
  alarmActive = false;
  
  // Initialize simulation offsets
  simTempOffset = 0;
  simHumOffset = 0;
  simMoistOffset = 0;
  simNOffset = 0;
  simPOffset = 0;
  simKOffset = 0;
  simECOffset = 0;
  
  // Initialize previous raw values
  prevRawTemp = 25.0f;
  prevRawHum = 50.0f;
  prevRawMoist = 2048;
  prevRawN = 2048;
  prevRawP = 2048;
  prevRawK = 2048;
  prevRawEC = 2048;
  
  // Initialize servo states
  servoFanPos = 90; servoFanDir = 1;
  servoWaterPos = 90; servoWaterDir = 1;
  servoNutrientPos = 90; servoNutrientDir = 1;
  
  // Default to auto mode
  isAutoMode = true;
  
  // Default profile
  profile = &PROFILE_LETTUCE;
}

PlantZone::~PlantZone() {
  if (dht) delete dht;
  if (ringHeat) delete ringHeat;
  if (ringMist) delete ringMist;
  if (ringGrow) delete ringGrow;
}

void PlantZone::begin() {
  // Initialize shared DHT22 (only once for all zones)
  if (sharedDHT == nullptr) {
    sharedDHT = new DHT(pinDHT, DHT22);
    sharedDHT->begin();
    Serial.println("[Shared] DHT22 initialized");
  }
  dht = sharedDHT;  // Point to shared sensor
  
  // Initialize LED rings (12 pixels each)
  ringHeat = new Adafruit_NeoPixel(12, pinRingHeat, NEO_GRB + NEO_KHZ800);
  ringMist = new Adafruit_NeoPixel(12, pinRingMist, NEO_GRB + NEO_KHZ800);
  ringGrow = new Adafruit_NeoPixel(12, pinRingGrow, NEO_GRB + NEO_KHZ800);
  
  ringHeat->begin();
  ringMist->begin();
  ringGrow->begin();
  
  ringHeat->clear(); ringHeat->show();
  ringMist->clear(); ringMist->show();
  ringGrow->clear(); ringGrow->show();
  
  // Initialize LDR pin (shared, but pinMode is harmless to call multiple times)
  pinMode(pinLDR, INPUT);
  
  // Initialize buzzer pin
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, LOW);
  
  // Stop all servos at center position
  pwm->setPWM(pwmChFan, 0, 375);
  pwm->setPWM(pwmChWater, 0, 375);
  pwm->setPWM(pwmChNutrient, 0, 375);
  
  Serial.printf("PlantZone %d: Initialized (Shared Environment)\n", zoneId + 1);
}

void PlantZone::setProfile(PlantProfile* newProfile) {
  profile = newProfile;
  Serial.printf("Zone %d: Profile changed to %s\n", zoneId + 1, profile->name);
}

// ==================== SENSOR READING ====================

void PlantZone::readSensors() {
  // Read DHT22 (Air temperature & humidity)
  float rawTemp = dht->readTemperature();
  float rawHum = dht->readHumidity();
  
  if (!isnan(rawTemp)) {
    float tempDelta = abs(rawTemp - prevRawTemp);
    if (tempDelta > 2.0f) {
      simTempOffset = 0;
      temperature = rawTemp;
    } else {
      temperature = rawTemp + simTempOffset;
    }
    prevRawTemp = rawTemp;
  }
  
  if (!isnan(rawHum)) {
    float humDelta = abs(rawHum - prevRawHum);
    if (humDelta > 5.0f) {
      simHumOffset = 0;
      humidity = rawHum;
    } else {
      humidity = rawHum + simHumOffset;
    }
    prevRawHum = rawHum;
  }
  
  // Clamp air values
  temperature = constrain(temperature, 0, 60);
  humidity = constrain(humidity, 0, 100);
  
  // Read LDR (Light level - inverted)
  int ldrRaw = analogRead(pinLDR);
  lightLevel = map(ldrRaw, 0, 4095, 100, 0);
  lightLevel = constrain(lightLevel, 0, 100);
  
  // Read soil sensors via MUX (with hybrid detection)
  int rawMoist = mux->readRaw(MUX_MOISTURE);
  if (abs(rawMoist - prevRawMoist) > 200) {
    simMoistOffset = 0;
  }
  prevRawMoist = rawMoist;
  moisture = map(rawMoist, 0, 4095, 0, 100) + (int)simMoistOffset;
  moisture = constrain(moisture, 0, 100);
  
  // Soil pH (direct reading, no simulation)
  soilPH = mux->readSoilPH();
  
  // NPK with simulation
  int rawN = mux->readRaw(MUX_NITROGEN);
  if (abs(rawN - prevRawN) > 100) simNOffset = 0;
  prevRawN = rawN;
  nitrogen = map(rawN, 0, 4095, 0, 500) + (int)simNOffset;
  nitrogen = constrain(nitrogen, 0, 500);
  
  int rawP = mux->readRaw(MUX_PHOSPHORUS);
  if (abs(rawP - prevRawP) > 100) simPOffset = 0;
  prevRawP = rawP;
  phosphorus = map(rawP, 0, 4095, 0, 500) + (int)simPOffset;
  phosphorus = constrain(phosphorus, 0, 500);
  
  int rawK = mux->readRaw(MUX_POTASSIUM);
  if (abs(rawK - prevRawK) > 100) simKOffset = 0;
  prevRawK = rawK;
  potassium = map(rawK, 0, 4095, 0, 500) + (int)simKOffset;
  potassium = constrain(potassium, 0, 500);
  
  // Hydro EC with simulation
  int rawEC = mux->readRaw(MUX_EC);
  if (abs(rawEC - prevRawEC) > 100) simECOffset = 0;
  prevRawEC = rawEC;
  hydroEC = (rawEC / 4095.0f * 3.0f) + simECOffset;
  hydroEC = constrain(hydroEC, 0.0f, 3.0f);
}

// ==================== ACTUATOR CONTROL ====================

void PlantZone::controlFan() {
  if (!isAutoMode) return;
  
  if (temperature > profile->tempMax) {
    fanActive = true;
  } else if (temperature < profile->tempMax - TEMP_HYSTERESIS) {
    fanActive = false;
  }
  
  if (fanActive) {
    servoFanPos += servoFanDir * 15;
    if (servoFanPos >= 180) { servoFanPos = 180; servoFanDir = -1; }
    if (servoFanPos <= 0) { servoFanPos = 0; servoFanDir = 1; }
    pwm->setPWM(pwmChFan, 0, map(servoFanPos, 0, 180, 150, 600));
    simTempOffset -= TEMP_COOLING_RATE;
  } else {
    servoFanPos = 90;
    pwm->setPWM(pwmChFan, 0, 375);
    if (simTempOffset < 0) simTempOffset += TEMP_RECOVERY_RATE;
  }
}

void PlantZone::controlHeat() {
  if (!isAutoMode) return;
  
  if (temperature < profile->tempMin) {
    heatActive = true;
  } else if (temperature > profile->tempMin + TEMP_HYSTERESIS) {
    heatActive = false;
  }
  
  if (heatActive) {
    for (int i = 0; i < ringHeat->numPixels(); i++) {
      ringHeat->setPixelColor(i, ringHeat->Color(255, 0, 0));
    }
    simTempOffset += TEMP_HEATING_RATE;
  } else {
    ringHeat->clear();
    if (simTempOffset > 0) simTempOffset -= TEMP_RECOVERY_RATE;
  }
  ringHeat->show();
}

void PlantZone::controlMist() {
  if (!isAutoMode) return;
  
  if (humidity < profile->humidityMin) {
    mistActive = true;
  } else if (humidity > profile->humidityMin + HUM_HYSTERESIS) {
    mistActive = false;
  }
  
  if (mistActive) {
    for (int i = 0; i < ringMist->numPixels(); i++) {
      ringMist->setPixelColor(i, ringMist->Color(0, 255, 255));
    }
    simHumOffset += HUM_INCREASE_RATE;
  } else {
    ringMist->clear();
    if (simHumOffset > 0) simHumOffset -= HUM_DECREASE_RATE;
  }
  ringMist->show();
}

void PlantZone::controlGrow() {
  if (!isAutoMode) return;
  
  if (lightLevel < profile->lightMin) {
    growActive = true;
  } else if (lightLevel > profile->lightMin + LIGHT_HYSTERESIS) {
    growActive = false;
  }
  
  if (growActive) {
    for (int i = 0; i < ringGrow->numPixels(); i++) {
      ringGrow->setPixelColor(i, ringGrow->Color(255, 0, 255));
    }
  } else {
    ringGrow->clear();
  }
  ringGrow->show();
}

void PlantZone::controlWater() {
  if (!isAutoMode) return;
  
  if (moisture < profile->moistureMin) {
    waterActive = true;
  } else if (moisture > profile->moistureMin + MOIST_HYSTERESIS) {
    waterActive = false;
  }
  
  if (waterActive) {
    servoWaterPos += servoWaterDir * 15;
    if (servoWaterPos >= 180) { servoWaterPos = 180; servoWaterDir = -1; }
    if (servoWaterPos <= 0) { servoWaterPos = 0; servoWaterDir = 1; }
    pwm->setPWM(pwmChWater, 0, map(servoWaterPos, 0, 180, 150, 600));
    simMoistOffset += MOIST_INCREASE_RATE;
  } else {
    servoWaterPos = 90;
    pwm->setPWM(pwmChWater, 0, 375);
    if (simMoistOffset > 0) simMoistOffset -= MOIST_DECREASE_RATE;
  }
}

void PlantZone::controlNutrient() {
  if (!isAutoMode) return;
  if (alarmActive) return;  // Safety lockout
  
  if (hydroEC < profile->ecMin) {
    nutrientActive = true;
  } else if (hydroEC > profile->ecMax - EC_HYSTERESIS) {
    nutrientActive = false;
  }
  
  if (nutrientActive) {
    servoNutrientPos += servoNutrientDir * 15;
    if (servoNutrientPos >= 180) { servoNutrientPos = 180; servoNutrientDir = -1; }
    if (servoNutrientPos <= 0) { servoNutrientPos = 0; servoNutrientDir = 1; }
    pwm->setPWM(pwmChNutrient, 0, map(servoNutrientPos, 0, 180, 150, 600));
    simECOffset += EC_INCREASE_RATE;
  } else {
    servoNutrientPos = 90;
    pwm->setPWM(pwmChNutrient, 0, 375);
    if (simECOffset > 0) simECOffset -= EC_DECREASE_RATE;
  }
}

void PlantZone::checkAlarm() {
  if (soilPH < profile->phMin || soilPH > profile->phMax) {
    alarmActive = true;
  } else {
    alarmActive = false;
  }
}

void PlantZone::controlActuators() {
  checkAlarm();
  controlFan();
  controlHeat();
  controlMist();
  controlGrow();
  controlWater();
  controlNutrient();
  updateBuzzer();  // Zone-specific buzzer
}

void PlantZone::updateBuzzer() {
  static bool buzzerState = false;
  static unsigned long lastToggle = 0;
  
  if (alarmActive) {
    // Beep pattern: 200ms on, 200ms off
    if (millis() - lastToggle > 200) {
      buzzerState = !buzzerState;
      if (buzzerState) {
        tone(pinBuzzer, 2000);
      } else {
        noTone(pinBuzzer);
      }
      lastToggle = millis();
    }
  } else {
    if (buzzerState) {
      noTone(pinBuzzer);
      buzzerState = false;
    }
  }
}

// ==================== MANUAL CONTROL ====================

void PlantZone::setFan(bool on) {
  if (isAutoMode) return;
  fanActive = on;
  pwm->setPWM(pwmChFan, 0, on ? 400 : 375);
}

void PlantZone::setHeat(bool on) {
  if (isAutoMode) return;
  heatActive = on;
  if (on) {
    for (int i = 0; i < ringHeat->numPixels(); i++) {
      ringHeat->setPixelColor(i, ringHeat->Color(255, 0, 0));
    }
  } else {
    ringHeat->clear();
  }
  ringHeat->show();
}

void PlantZone::setMist(bool on) {
  if (isAutoMode) return;
  mistActive = on;
  if (on) {
    for (int i = 0; i < ringMist->numPixels(); i++) {
      ringMist->setPixelColor(i, ringMist->Color(0, 255, 255));
    }
  } else {
    ringMist->clear();
  }
  ringMist->show();
}

void PlantZone::setGrow(bool on) {
  if (isAutoMode) return;
  growActive = on;
  if (on) {
    for (int i = 0; i < ringGrow->numPixels(); i++) {
      ringGrow->setPixelColor(i, ringGrow->Color(255, 0, 255));
    }
  } else {
    ringGrow->clear();
  }
  ringGrow->show();
}

void PlantZone::setWater(bool on) {
  if (isAutoMode) return;
  waterActive = on;
  pwm->setPWM(pwmChWater, 0, on ? 400 : 375);
}

void PlantZone::setNutrient(bool on) {
  if (isAutoMode) return;
  nutrientActive = on;
  pwm->setPWM(pwmChNutrient, 0, on ? 400 : 375);
}

// ==================== DISPLAY ====================

void PlantZone::updateDisplay(int yOffset) {
  if (!tft) return;
  
  char buffer[50];
  
  // Zone header - 2x font
  tft->fillRect(0, yOffset, 240, 24, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(1);
  tft->setTextSize(2);
  sprintf(buffer, "VUNG %d: %s", zoneId + 1, profile->name);
  tft->drawString(buffer, 5, yOffset + 4);
  
  // Sensor data - 2x font, each on its own line
  int y = yOffset + 30;
  int lineHeight = 18;  // Height for 2x font
  
  // Temperature
  tft->setTextColor(TFT_GREEN, TFT_BLACK);
  sprintf(buffer, "Nhiet do: %.1f do C   ", temperature);
  tft->drawString(buffer, 5, y);
  
  // Humidity (Air)
  y += lineHeight;
  sprintf(buffer, "Do am KK: %.0f %%      ", humidity);
  tft->drawString(buffer, 5, y);
  
  // Light level
  y += lineHeight;
  sprintf(buffer, "Anh sang: %d %%       ", lightLevel);
  tft->drawString(buffer, 5, y);
  
  // Soil moisture
  y += lineHeight;
  tft->setTextColor(TFT_CYAN, TFT_BLACK);
  sprintf(buffer, "Do am dat: %d %%      ", moisture);
  tft->drawString(buffer, 5, y);
  
  // pH with color coding
  y += lineHeight;
  uint16_t phColor = TFT_GREEN;
  if (soilPH < profile->phMin || soilPH > profile->phMax) {
    phColor = TFT_RED;
  } else if (soilPH < profile->phMin + 0.5 || soilPH > profile->phMax - 0.5) {
    phColor = TFT_YELLOW;
  }
  tft->setTextColor(phColor, TFT_BLACK);
  sprintf(buffer, "pH dat: %.1f         ", soilPH);
  tft->drawString(buffer, 5, y);
  
  // NPK separated
  y += lineHeight;
  tft->setTextColor(TFT_MAGENTA, TFT_BLACK);
  sprintf(buffer, "N: %d mg/kg          ", nitrogen);
  tft->drawString(buffer, 5, y);

  y += lineHeight;
  sprintf(buffer, "P: %d mg/kg          ", phosphorus);
  tft->drawString(buffer, 5, y);

  y += lineHeight;
  sprintf(buffer, "K: %d mg/kg          ", potassium);
  tft->drawString(buffer, 5, y);

  // Hydro EC
  y += lineHeight;
  tft->setTextColor(TFT_ORANGE, TFT_BLACK);
  sprintf(buffer, "EC: %.2f mS/cm       ", hydroEC);
  tft->drawString(buffer, 5, y);
  
  // Alarm indicator
  if (alarmActive) {
    tft->fillRect(210, yOffset, 30, 24, TFT_RED);
    tft->setTextColor(TFT_WHITE, TFT_RED);
    tft->drawString("!", 218, yOffset + 4);
  } else {
    tft->fillRect(210, yOffset, 30, 24, TFT_DARKGREY);
  }
}

// ==================== MAIN UPDATE ====================

void PlantZone::update(int displayYOffset) {
  readSensors();
  controlActuators();
  updateDisplay(displayYOffset);
}
