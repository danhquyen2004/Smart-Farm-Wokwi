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
  ledRing = nullptr;
  
  // ESP32 DevKit V1 - Shared Environment Edition
  // All zones share the same environment sensors
  
  // Shared sensor pins (only Zone 0 initializes, Zone 1 reads same values)
  pinDHT = 25;      // Shared DHT22
  pinLDR = 36;      // Shared LDR (VP)
  
  // Zone-specific actuator pins
  if (zoneId == 0) {
    pinBuzzer = 4;   // Moved from 33 to free up ADC pin
    pinLedRing = 12;
  } else {
    pinBuzzer = 5;   // Moved from 32 to free up ADC pin
    pinLedRing = 13;
  }
  
  
  // PCA9685 channels
  pwmChFan = zoneId * 6;
  pwmChWater = zoneId * 6 + 1;
  pwmChN = zoneId * 6 + 2;
  pwmChP = zoneId * 6 + 3;
  pwmChK = zoneId * 6 + 4;
  pwmChNutrient = zoneId * 6 + 5;
  
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
  nActive = false;
  pActive = false;
  kActive = false;
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
  servoNPos = 90; servoNDir = 1;
  servoPPos = 90; servoPDir = 1;
  servoKPos = 90; servoKDir = 1;
  servoNutrientPos = 90; servoNutrientDir = 1;
  
  // Default to auto mode
  isAutoMode = true;
  
  // Default profile
  profile = &PROFILE_LETTUCE;
  
  // Performance optimization
  lastUpdateTFT = 0;
  lastReadDHT = 0;
  lastDispTemp = -999;
  lastDispHum = -999;
  lastDispLight = -999;
  lastDispMoist = -999;
  lastDispPH = -999;
  lastDispN = -999; lastDispP = -999; lastDispK = -999;
  lastDispEC = -999;
  lastDispAlarm = false;
}

PlantZone::~PlantZone() {
  if (dht) delete dht;
  if (ledRing) delete ledRing;
}

void PlantZone::begin() {
  // Initialize shared DHT22 (only once for all zones)
  if (sharedDHT == nullptr) {
    sharedDHT = new DHT(pinDHT, DHT22);
    sharedDHT->begin();
    Serial.println("[Shared] DHT22 initialized");
  }
  dht = sharedDHT;  // Point to shared sensor
  
  // Initialize daisy-chained LED rings
  ledRing = new Adafruit_NeoPixel(36, pinLedRing, NEO_GRB + NEO_KHZ800);
  ledRing->begin();
  delay(50);
  ledRing->clear(); 
  ledRing->show();
  delay(50);
  
  // Initialize LDR pin (shared, but pinMode is harmless to call multiple times)
  pinMode(pinLDR, INPUT);
  
  // Initialize buzzer pin
  pinMode(pinBuzzer, OUTPUT);
  digitalWrite(pinBuzzer, LOW);
  
  // Stop all servos at center position
  pwm->setPWM(pwmChFan, 0, 375);
  pwm->setPWM(pwmChWater, 0, 375);
  pwm->setPWM(pwmChN, 0, 375);
  pwm->setPWM(pwmChP, 0, 375);
  pwm->setPWM(pwmChK, 0, 375);
  pwm->setPWM(pwmChNutrient, 0, 375);
  
  Serial.printf("PlantZone %d: Initialized (Shared Environment)\n", zoneId + 1);
}

void PlantZone::setProfile(PlantProfile* newProfile) {
  profile = newProfile;
  Serial.printf("Zone %d: Profile changed to %s\n", zoneId + 1, profile->name);
}

// ==================== SENSOR READING ====================

void PlantZone::readSensors() {
  // Read DHT22 only every 2 seconds (Shared Environment optimization)
  if (millis() - lastReadDHT > 2000) {
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
    lastReadDHT = millis();
  }
  
  // Clamp air values
  temperature = constrain(temperature, -40, 80);
  humidity = constrain(humidity, 0, 100);
  
  // Read LDR (Light level - inverted)
  int ldrRaw = analogRead(pinLDR);
  lightLevel = map(ldrRaw, 0, 4095, 100, 0);
  lightLevel = constrain(lightLevel, 0, 100);
  
  // Read soil sensors DIRECTLY from ADC pins (no MUX - for Wokwi stability)
  // Pin assignments: Moisture=34, pH=35, N=32, P=33, K=39(VN), EC=26
  
  // Moisture (GPIO 34)
  int rawMoist = analogRead(34);
  if (abs(rawMoist - prevRawMoist) > 200) {
    simMoistOffset = 0;
  }
  prevRawMoist = rawMoist;
  moisture = map(rawMoist, 0, 4095, 0, 100) + (int)simMoistOffset;
  moisture = constrain(moisture, 0, 100);
  
  // Soil pH (GPIO 35) - 0 to 14 range
  int rawPH = analogRead(35);
  soilPH = rawPH / 4095.0f * 14.0f;
  soilPH = constrain(soilPH, 0.0f, 14.0f);
  
  // Nitrogen (GPIO 32)
  int rawN = analogRead(32);
  if (abs(rawN - prevRawN) > 100) simNOffset = 0;
  prevRawN = rawN;
  nitrogen = map(rawN, 0, 4095, 0, 500) + (int)simNOffset;
  nitrogen = constrain(nitrogen, 0, 500);
  
  // Phosphorus (GPIO 33)
  int rawP = analogRead(33);
  if (abs(rawP - prevRawP) > 100) simPOffset = 0;
  prevRawP = rawP;
  phosphorus = map(rawP, 0, 4095, 0, 500) + (int)simPOffset;
  phosphorus = constrain(phosphorus, 0, 500);
  
  // Potassium (GPIO 39 = VN)
  int rawK = analogRead(39);
  if (abs(rawK - prevRawK) > 100) simKOffset = 0;
  prevRawK = rawK;
  potassium = map(rawK, 0, 4095, 0, 500) + (int)simKOffset;
  potassium = constrain(potassium, 0, 500);
  
  // Hydro EC (GPIO 26)
  int rawEC = analogRead(26);
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
    for (int i = 0; i < 12; i++) {
      ledRing->setPixelColor(i, ledRing->Color(255, 0, 0));
    }
    simTempOffset += TEMP_HEATING_RATE;
  } else {
    for (int i = 0; i < 12; i++) ledRing->setPixelColor(i, 0);
    if (simTempOffset > 0) simTempOffset -= TEMP_RECOVERY_RATE;
  }
}

void PlantZone::controlMist() {
  if (!isAutoMode) return;
  
  if (humidity < profile->humidityMin) {
    mistActive = true;
  } else if (humidity > profile->humidityMin + HUM_HYSTERESIS) {
    mistActive = false;
  }
  
  if (mistActive) {
    for (int i = 12; i < 24; i++) {
      ledRing->setPixelColor(i, ledRing->Color(0, 255, 255));
    }
    simHumOffset += HUM_INCREASE_RATE;
  } else {
    for (int i = 12; i < 24; i++) ledRing->setPixelColor(i, 0);
    if (simHumOffset > 0) simHumOffset -= HUM_DECREASE_RATE;
  }
}

void PlantZone::controlGrow() {
  if (!isAutoMode) return;
  
  if (lightLevel < profile->lightMin) {
    growActive = true;
  } else if (lightLevel > profile->lightMin + LIGHT_HYSTERESIS) {
    growActive = false;
  }
  
  if (growActive) {
    for (int i = 24; i < 36; i++) {
      ledRing->setPixelColor(i, ledRing->Color(255, 0, 255));
    }
  } else {
    for (int i = 24; i < 36; i++) ledRing->setPixelColor(i, 0);
  }
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

void PlantZone::controlN() {
  if (!isAutoMode) return;
  
  if (nitrogen < profile->nitrogenMin) {
    nActive = true;
  } else if (nitrogen > profile->nitrogenMin + NUTRIENT_HYSTERESIS) {
    nActive = false;
  }
  
  if (nActive) {
    servoNPos += servoNDir * 15;
    if (servoNPos >= 180) { servoNPos = 180; servoNDir = -1; }
    if (servoNPos <= 0) { servoNPos = 0; servoNDir = 1; }
    pwm->setPWM(pwmChN, 0, map(servoNPos, 0, 180, 150, 600));
    simNOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoNPos = 90;
    pwm->setPWM(pwmChN, 0, 375);
    if (simNOffset > 0) simNOffset -= NUTRIENT_DECREASE_RATE;
  }
}

void PlantZone::controlP() {
  if (!isAutoMode) return;
  
  if (phosphorus < profile->phosphorusMin) {
    pActive = true;
  } else if (phosphorus > profile->phosphorusMin + NUTRIENT_HYSTERESIS) {
    pActive = false;
  }
  
  if (pActive) {
    servoPPos += servoPDir * 15;
    if (servoPPos >= 180) { servoPPos = 180; servoPDir = -1; }
    if (servoPPos <= 0) { servoPPos = 0; servoPDir = 1; }
    pwm->setPWM(pwmChP, 0, map(servoPPos, 0, 180, 150, 600));
    simPOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoPPos = 90;
    pwm->setPWM(pwmChP, 0, 375);
    if (simPOffset > 0) simPOffset -= NUTRIENT_DECREASE_RATE;
  }
}

void PlantZone::controlK() {
  if (!isAutoMode) return;
  
  if (potassium < profile->potassiumMin) {
    kActive = true;
  } else if (potassium > profile->potassiumMin + NUTRIENT_HYSTERESIS) {
    kActive = false;
  }
  
  if (kActive) {
    servoKPos += servoKDir * 15;
    if (servoKPos >= 180) { servoKPos = 180; servoKDir = -1; }
    if (servoKPos <= 0) { servoKPos = 0; servoKDir = 1; }
    pwm->setPWM(pwmChK, 0, map(servoKPos, 0, 180, 150, 600));
    simKOffset += NUTRIENT_INCREASE_RATE;
  } else {
    servoKPos = 90;
    pwm->setPWM(pwmChK, 0, 375);
    if (simKOffset > 0) simKOffset -= NUTRIENT_DECREASE_RATE;
  }
}

void PlantZone::controlNutrient() {
  if (!isAutoMode) return;
  
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
  controlN();
  controlP();
  controlK();
  controlNutrient();
  updateBuzzer();  // Zone-specific buzzer
  
  // Update LEDs once per loop for efficiency
  ledRing->show();
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
    for (int i = 0; i < 12; i++) {
      ledRing->setPixelColor(i, ledRing->Color(255, 0, 0));
    }
  } else {
    for (int i = 0; i < 12; i++) ledRing->setPixelColor(i, 0);
  }
  ledRing->show();
}

void PlantZone::setMist(bool on) {
  if (isAutoMode) return;
  mistActive = on;
  if (on) {
    for (int i = 12; i < 24; i++) {
      ledRing->setPixelColor(i, ledRing->Color(0, 255, 255));
    }
  } else {
    for (int i = 12; i < 24; i++) ledRing->setPixelColor(i, 0);
  }
  ledRing->show();
}

void PlantZone::setGrow(bool on) {
  if (isAutoMode) return;
  growActive = on;
  if (on) {
    for (int i = 24; i < 36; i++) {
      ledRing->setPixelColor(i, ledRing->Color(255, 0, 255));
    }
  } else {
    for (int i = 24; i < 36; i++) ledRing->setPixelColor(i, 0);
  }
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
  
  // Only update TFT once every 1000ms to reduce simulation lag
  if (millis() - lastUpdateTFT < 1000) return;
  lastUpdateTFT = millis();
  
  // Header - Size 2
  tft->fillRect(0, yOffset, 240, 24, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(1);
  tft->setTextSize(2);
  sprintf(buffer, "VUNG %d: %s", zoneId + 1, profile->name);
  tft->drawString(buffer, 5, yOffset + 4);
  
  int y = yOffset + 30;
  int lineHeight = 18;
  
  // Temperature - Only redraw if changed significantly
  if (abs(temperature - lastDispTemp) > 0.1f) {
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    sprintf(buffer, "Nhiet do: %.1f do C   ", temperature);
    tft->drawString(buffer, 5, y);
    lastDispTemp = temperature;
  }
  
  // Humidity
  y += lineHeight;
  if (abs(humidity - lastDispHum) > 0.5f) {
    tft->setTextColor(TFT_BLUE, TFT_BLACK);
    sprintf(buffer, "Do am KK: %.0f %%      ", humidity);
    tft->drawString(buffer, 5, y);
    lastDispHum = humidity;
  }
  
  // Light
  y += lineHeight;
  if (abs(lightLevel - lastDispLight) > 1) {
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    sprintf(buffer, "Anh sang: %d %%       ", lightLevel);
    tft->drawString(buffer, 5, y);
    lastDispLight = lightLevel;
  }
  
  // Soil moisture
  y += lineHeight;
  if (abs(moisture - lastDispMoist) > 1) {
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    sprintf(buffer, "Do am dat: %d %%      ", moisture);
    tft->drawString(buffer, 5, y);
    lastDispMoist = moisture;
  }
  
  // pH
  y += lineHeight;
  if (abs(soilPH - lastDispPH) > 0.1f) {
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    sprintf(buffer, "pH dat: %.1f         ", soilPH);
    tft->drawString(buffer, 5, y);
    lastDispPH = soilPH;
  }
  
  // N
  y += lineHeight;
  if (nitrogen != lastDispN) {
    tft->setTextColor(TFT_MAGENTA, TFT_BLACK);
    sprintf(buffer, "N: %d mg/kg          ", nitrogen);
    tft->drawString(buffer, 5, y);
    lastDispN = nitrogen;
  }

  // P
  y += lineHeight;
  if (phosphorus != lastDispP) {
    tft->setTextColor(TFT_RED, TFT_BLACK);
    sprintf(buffer, "P: %d mg/kg          ", phosphorus);
    tft->drawString(buffer, 5, y);
    lastDispP = phosphorus;
  }

  // K
  y += lineHeight;
  if (potassium != lastDispK) {
    tft->setTextColor(TFT_PINK, TFT_BLACK);
    sprintf(buffer, "K: %d mg/kg          ", potassium);
    tft->drawString(buffer, 5, y);
    lastDispK = potassium;
  }

  // EC
  y += lineHeight;
  if (abs(hydroEC - lastDispEC) > 0.05f) {
    tft->setTextColor(TFT_ORANGE, TFT_BLACK);
    sprintf(buffer, "EC: %.2f mS/cm       ", hydroEC);
    tft->drawString(buffer, 5, y);
    lastDispEC = hydroEC;
  }
  
  // Alarm indicator
  if (alarmActive != lastDispAlarm) {
    if (alarmActive) {
      tft->fillRect(210, yOffset, 30, 24, TFT_RED);
      tft->setTextColor(TFT_WHITE, TFT_RED);
      tft->drawString("!", 218, yOffset + 4);
    } else {
      tft->fillRect(210, yOffset, 30, 24, TFT_DARKGREY);
    }
    lastDispAlarm = alarmActive;
  }
}

// ==================== MAIN UPDATE ====================

void PlantZone::update(int displayYOffset) {
  readSensors();
  controlActuators();
  updateDisplay(displayYOffset);
}
