#include "MuxManager.h"

MuxManager::MuxManager() {
  // Constructor - pins will be initialized in begin()
}

void MuxManager::begin() {
  // Configure select pins as outputs
  pinMode(PIN_MUX_S0, OUTPUT);
  pinMode(PIN_MUX_S1, OUTPUT);
  pinMode(PIN_MUX_S2, OUTPUT);
  pinMode(PIN_MUX_S3, OUTPUT);
  
  // Configure MUX1 COM pin as input (ADC for analog sensors)
  pinMode(PIN_MUX1_COM, INPUT);
  
  // Start with channel 0 selected
  selectChannel(0);
  
  Serial.println("MuxManager: Initialized (ESP32 DevKit V1)");
  Serial.println("  MUX1 (Analog) on GPIO34");
  Serial.println("  S0-S3: GPIO 16, 17, 5, 4");
}

void MuxManager::selectChannel(uint8_t channel) {
  // Channel is 4-bit value (0-15) for 16-channel CD74HC4067
  digitalWrite(PIN_MUX_S0, channel & 0x01);
  digitalWrite(PIN_MUX_S1, (channel >> 1) & 0x01);
  digitalWrite(PIN_MUX_S2, (channel >> 2) & 0x01);
  digitalWrite(PIN_MUX_S3, (channel >> 3) & 0x01);
  
  // Wait for signal to settle
  delayMicroseconds(SETTLING_TIME_US);
}

// ==================== MUX1 (Analog Sensors) ====================

int MuxManager::readRaw(uint8_t channel) {
  if (channel > 15) return 0;
  
  selectChannel(channel);
  return analogRead(PIN_MUX1_COM);
}

int MuxManager::readPercent(uint8_t channel) {
  int raw = readRaw(channel);
  return map(raw, 0, 4095, 0, 100);
}

float MuxManager::readMapped(uint8_t channel, float minVal, float maxVal) {
  int raw = readRaw(channel);
  return minVal + (maxVal - minVal) * raw / 4095.0f;
}

int MuxManager::readMoisture() {
  return readPercent(MUX_MOISTURE);
}

float MuxManager::readSoilPH() {
  return readMapped(MUX_PH_SOIL, 0.0f, 14.0f);
}

int MuxManager::readNitrogen() {
  int raw = readRaw(MUX_NITROGEN);
  return map(raw, 0, 4095, 0, 500);
}

int MuxManager::readPhosphorus() {
  int raw = readRaw(MUX_PHOSPHORUS);
  return map(raw, 0, 4095, 0, 500);
}

int MuxManager::readPotassium() {
  int raw = readRaw(MUX_POTASSIUM);
  return map(raw, 0, 4095, 0, 500);
}

float MuxManager::readEC() {
  return readMapped(MUX_EC, 0.0f, 3.0f);
}
