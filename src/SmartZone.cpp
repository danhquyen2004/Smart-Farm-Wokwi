#include "SmartZone.h"

// Static member definition
TFT_eSPI* SmartZone::tft = nullptr;

SmartZone::SmartZone(int id, ZoneType type, PlantProfile* profile, ProfileThresholds* thresh) {
  zoneID = id;
  zoneType = type;
  currentProfile = profile;
  thresholds = thresh;
  ecAlarmActive = false;
  
  dht = nullptr;
  oneWire = nullptr;
  ds18b20 = nullptr;
  ringGrow = nullptr;
  ringHeat = nullptr;
  ringMist = nullptr;
  
  // Set pins
  if (zoneType == ZONE_AIR) {
    pins.dht = 26; 
    pins.ldr = 34;
    pins.srvFan = 13;
    pins.ringGrow = 27;
    pins.ringHeat = 12;
    pins.ringMist = 14;
  } else if (zoneType == ZONE_SOIL) {
    pins.potMoist = 36;
    pins.potN = 39;
    pins.potP = 35;
    pins.potK = 25;
    pins.potPH = 32;
    pins.potEC = 33;
    pins.srvWater = 16;
    pins.srvN = 17;
    pins.srvP = 5;
    pins.srvK = 1;
    pins.buzzer = 3;
  } else if (zoneType == ZONE_HYDRO) {
    pins.ds18b20 = 21;
    pins.potEcHydro = 22;
    pins.potPhHydro = 0;
    pins.srvChiller = 17;
    pins.srvNutrient = 16;
  }
}

SmartZone::~SmartZone() {
  if (dht) delete dht;
  if (ringGrow) delete ringGrow;
  if (ringHeat) delete ringHeat;
  if (ringMist) delete ringMist;
  if (oneWire) delete oneWire;
  if (ds18b20) delete ds18b20;
}

void SmartZone::setTFT(TFT_eSPI* tftInstance) {
  tft = tftInstance;
}

void SmartZone::begin() {
  if (zoneType == ZONE_AIR) initZoneA();
  else if (zoneType == ZONE_SOIL) initZoneB();
  else if (zoneType == ZONE_HYDRO) initZoneC();
}

void SmartZone::initZoneA() {
  dht = new DHT(pins.dht, DHT22);
  dht->begin();
  srvFan.attach(pins.srvFan); srvFan.write(0);
  ringGrow = new Adafruit_NeoPixel(12, pins.ringGrow, NEO_GRB + NEO_KHZ800);
  ringHeat = new Adafruit_NeoPixel(12, pins.ringHeat, NEO_GRB + NEO_KHZ800);
  ringMist = new Adafruit_NeoPixel(12, pins.ringMist, NEO_GRB + NEO_KHZ800);
  ringGrow->begin(); ringGrow->show();
  ringHeat->begin(); ringHeat->show();
  ringMist->begin(); ringMist->show();
}

void SmartZone::initZoneB() {
  srvWater.attach(pins.srvWater); srvWater.write(0);
  srvN.attach(pins.srvN); srvN.write(0);
  srvP.attach(pins.srvP); srvP.write(0);
  srvK.attach(pins.srvK); srvK.write(0);
  pinMode(pins.buzzer, OUTPUT);
}

void SmartZone::initZoneC() {
  oneWire = new OneWire(pins.ds18b20);
  ds18b20 = new DallasTemperature(oneWire);
  ds18b20->begin();
  srvChiller.attach(pins.srvChiller); srvChiller.write(0);
  srvNutrient.attach(pins.srvNutrient); srvNutrient.write(0);
}

// Drawing Helpers - Simplified using Font 2 everywhere

void SmartZone::drawValue(int x, int y, const char* label, float value, const char* unit, uint16_t color) {
  if (!tft) return;
  
  // Format Value string
  char valStr[16];
  if (strstr(unit, "mS") || strstr(unit, "pH") || strstr(unit, "C")) sprintf(valStr, "%.1f", value);
  else sprintf(valStr, "%.0f", value);
  
  // Clear layout:
  // Label: .......... Value Unit
  
  // Draw Label (Left aligned)
  tft->setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft->drawString(label, x, y, 1); 
  
  // Determine value X position dynamically based on label width
  int labelW = tft->textWidth(label, 1);
  int valX = x + labelW + 5; 
  
  // Clear value area (calculate needed width roughly)
  tft->fillRect(valX, y, 50, 16, TFT_BLACK); // Smaller clear area for compact layout
  
  tft->setTextColor(color, TFT_BLACK);
  tft->drawString(valStr, valX, y, 1);
  
  // Draw Unit
  int unitX = valX + tft->textWidth(valStr, 1) + 3;
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->drawString(unit, unitX, y, 1);
}

// Not used anymore but kept to avoid build error with header if not updated
void SmartZone::drawStatus(int x, int y, const char* label, bool active, uint16_t colorActive) {
  // Disabled as per user request
}

void SmartZone::drawLabel(int x, int y, const char* label, uint16_t color) {
  if (!tft) return;
  tft->setTextColor(color, TFT_BLACK);
  tft->drawString(label, x, y, 1);
}

void SmartZone::run() {
  int h = 106; 
  int yOffset = (zoneID - 1) * h;
  
  // Header
  tft->fillRect(0, yOffset, 240, 20, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  
  String zoneName;
  if (zoneType == ZONE_AIR) zoneName = "KHU VUC A: KHONG KHI";
  else if (zoneType == ZONE_SOIL) zoneName = "KHU VUC B: DAT";
  else zoneName = "KHU VUC C: THUY CANH";
  
  tft->drawString(zoneName, 5, yOffset + 3, 1);
  // tft->drawRightString(getProfileName(*currentProfile), 235, yOffset + 2, 2); // Hide profile to save space if needed
  
  if (zoneType == ZONE_AIR) runZoneA();
  else if (zoneType == ZONE_SOIL) runZoneB();
  else if (zoneType == ZONE_HYDRO) runZoneC();
  
  // Line
  tft->drawFastHLine(0, yOffset + h - 1, 240, TFT_WHITE);
}

void SmartZone::runZoneA() {
  int h = 106;
  int yBase = (zoneID - 1) * h + 25;
  int lineH = 25; 
  
  // Read Raw Sensors
  float realTemp = dht->readTemperature();
  float realHum = dht->readHumidity();
  int lux = map(analogRead(pins.ldr), 0, 4095, 100, 0);
  
  // --- SIMULATION LOGIC ---
  if (!isnan(realTemp)) {
    // Temperature Simulation
    if (fanState) simTempOffset -= 0.9;     // Cooling fast (Was 0.3)
    else if (heatState) simTempOffset += 0.9; // Heating fast (Was 0.3)
    else {
      // Natural recovery to environment
      if (simTempOffset > 0.1) simTempOffset -= 0.15;
      else if (simTempOffset < -0.1) simTempOffset += 0.15;
      else simTempOffset = 0.0;
    }
    
    // Limits removed as per user request
    // if(simTempOffset > 50) simTempOffset = 50;
    // if(simTempOffset < -50) simTempOffset = -50;
  }
  
  if (!isnan(realHum)) {
    // Humidity Simulation
    if (mistState) simHumOffset += 1.5; // Humidify fast (Was 0.5)
    else {
      // Dry out naturally
      if (simHumOffset > 0.1) simHumOffset -= 0.3;
      else if (simHumOffset < -0.1) simHumOffset += 0.3; 
      else simHumOffset = 0.0;
    }
  }
  
  // Calc Display Values
  float dispTemp = realTemp + simTempOffset;
  float dispHum = realHum + simHumOffset;
  if (dispHum > 100) dispHum = 100; // Cap humidity
  
  // Draw
  if(!isnan(dispTemp)) drawValue(10, yBase, "Nhiet Do:", dispTemp, "C", TFT_GREEN);
  if(!isnan(dispHum)) drawValue(10, yBase + lineH, "Do Am:", dispHum, "%", TFT_CYAN);
  drawValue(10, yBase + lineH*2, "Anh Sang:", (float)lux, "%", TFT_YELLOW);
  
  // Control Logic (Hysteresis based on DISPLAY/SIMULATED value)
  if (!isnan(dispTemp)) {
    if (dispTemp > thresholds->tempMax) fanState = true;
    else if (dispTemp < thresholds->tempMax - 2.0) fanState = false;
    
    if (dispTemp < thresholds->tempMin) heatState = true;
    else if (dispTemp > thresholds->tempMin + 2.0) heatState = false;
  }
  
  if (!isnan(dispHum)) {
    if (dispHum < thresholds->humidityMin) mistState = true;
    else if (dispHum > thresholds->humidityMin + 5.0) mistState = false;
  }
  
  // Light (No simulation needed, immediate response usually, or sim offset could be added later)
  if (lux < thresholds->lightMin) growState = true;
  else if (lux > thresholds->lightMin + 10) growState = false;
  
  // Actuate
  srvFan.write(fanState ? 90 : 0); // ON=90 (Doc), OFF=0 (Ngang)
  colorWipe(ringHeat, heatState ? ringHeat->Color(255,0,0) : 0);
  colorWipe(ringMist, mistState ? ringMist->Color(0,255,255) : 0);
  colorWipe(ringGrow, growState ? ringGrow->Color(255,0,255) : 0);
}

void SmartZone::runZoneB() {
  int h = 106;
  int yBase = (zoneID - 1) * h + 25;
  int lineH = 25; // Increased spacing
  
  int moist = map(analogRead(pins.potMoist), 0, 4095, 0, 100);
  float ph = map(analogRead(pins.potPH), 0, 4095, 0, 140) / 10.0;
  float ec = map(analogRead(pins.potEC), 0, 4095, 0, 50) / 10.0;
  
  int nVal = map(analogRead(pins.potN), 0, 4095, 0, 100);
  int pVal = map(analogRead(pins.potP), 0, 4095, 0, 100);
  int kVal = map(analogRead(pins.potK), 0, 4095, 0, 100);
  
  drawValue(10, yBase, "Do Am:", (float)moist, "%", TFT_BLUE);
  
  // Row 2: pH and EC on same line significantly spaced
  drawValue(10, yBase + lineH, "pH:", ph, "", TFT_GREEN);
  drawValue(120, yBase + lineH, "EC:", ec, "mS", (ec > thresholds->ecMax) ? TFT_RED : TFT_ORANGE);
  
  // Row 3: N P K compact
  // "N: 100mg" ~ 45px width
  drawValue(10, yBase + lineH*2, "N:", (float)nVal, "mg", TFT_PURPLE);
  drawValue(80, yBase + lineH*2, "P:", (float)pVal, "mg", TFT_PURPLE);
  drawValue(150, yBase + lineH*2, "K:", (float)kVal, "mg", TFT_PURPLE);
  
  // Alarm Logic (Overlay if alarm)
  if (ec > thresholds->ecMax) {
    if(!ecAlarmActive) { ecAlarmActive = true; tone(pins.buzzer, 1000); }
    tft->fillRoundRect(40, yBase + 10, 160, 60, 5, TFT_RED);
    tft->setTextColor(TFT_WHITE, TFT_RED);
    tft->drawCentreString("! CANH BAO EC !", 120, yBase + 25, 2);
    tft->drawCentreString("XA DINH DUONG", 120, yBase + 45, 1);
    
    srvN.write(0); srvP.write(0); srvK.write(0);
    srvWater.write(90);
  } else {
    if(ecAlarmActive) { 
      ecAlarmActive = false; 
      noTone(pins.buzzer); 
      // Clear alarm box area carefully or just let simple redraw handle it in next loop (background overwrite in drawValue handles most, but we need to clear the big box)
      tft->fillRect(40, yBase + 10, 160, 60, TFT_BLACK); 
    }
    bool water = (moist < thresholds->soilMoistureMin);
    srvWater.write(water ? 90 : 0);
  }
}

void SmartZone::runZoneC() {
  int h = 106;
  int yBase = (zoneID - 1) * h + 25;
  int lineH = 20;
  
  ds18b20->requestTemperatures();
  float temp = ds18b20->getTempCByIndex(0);
  float ec = map(analogRead(pins.potEcHydro), 0, 4095, 0, 50) / 10.0;
  float ph = map(analogRead(pins.potPhHydro), 0, 4095, 0, 140) / 10.0;
  
  drawValue(10, yBase, "Nhiet Nuoc:", temp, "C", TFT_BLUE);
  drawValue(10, yBase + lineH, "EC Thuy:", ec, "mS", TFT_ORANGE);
  drawValue(10, yBase + lineH*2, "pH Thuy:", ph, "", TFT_GREEN);
  
  bool chill = (temp > thresholds->waterTempMax);
  bool nutr = (ec < thresholds->hydroEcMin);
  srvChiller.write(chill ? 180 : 0);
  srvNutrient.write(nutr ? 90 : 0);
}

void SmartZone::updateProfile(PlantProfile* profile, ProfileThresholds* thresh) {
  currentProfile = profile;
  thresholds = thresh;
}

void SmartZone::colorWipe(Adafruit_NeoPixel* ring, uint32_t color) {
  if (ring) {
    for(int i=0; i<ring->numPixels(); i++) ring->setPixelColor(i, color);
    ring->show();
  }
}
