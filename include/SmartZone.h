#ifndef SMARTZONE_H
#define SMARTZONE_H

#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Changed from LiquidCrystal_I2C
#include <DHT.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PlantProfile.h"

class SmartZone {
private:
  int zoneID;
  ZoneType zoneType;
  PlantProfile* currentProfile;
  ProfileThresholds* thresholds;
  
  // Hardware objects
  DHT* dht;
  static TFT_eSPI* tft; // Shared TFT object
  Adafruit_NeoPixel* ringGrow;
  Adafruit_NeoPixel* ringHeat;
  Adafruit_NeoPixel* ringMist;
  
  // Zone-specific hardware
  Servo srvFan;
  Servo srvWater;
  Servo srvN, srvP, srvK;
  Servo srvChiller;
  Servo srvNutrient;
  
  OneWire* oneWire;
  DallasTemperature* ds18b20;
  
  // Pin configuration
  struct Pins {
    uint8_t dht;
    uint8_t ldr;
    uint8_t ds18b20;
    uint8_t potMoist, potN, potP, potK, potPH, potEC;
    uint8_t potEcHydro, potPhHydro;
    uint8_t srvFan;
    uint8_t srvWater, srvN, srvP, srvK;
    uint8_t srvChiller, srvNutrient;
    uint8_t ringGrow, ringHeat, ringMist;
    uint8_t buzzer;
  } pins;
  
  bool ecAlarmActive;
  
  // Zone A States (Hysteresis)
  bool fanState = false;
  bool heatState = false;
  bool mistState = false;
  bool growState = false;
  
  // Simulation Offsets
  float simTempOffset = 0.0;
  float simHumOffset = 0.0;
  
  // Helper functions
  void colorWipe(Adafruit_NeoPixel* ring, uint32_t color);
  void initZoneA();
  void initZoneB();
  void initZoneC();
  void runZoneA();
  void runZoneB();
  void runZoneC();
  
  // Drawing helpers
  void drawLabel(int x, int y, const char* label, uint16_t color);
  void drawValue(int x, int y, const char* label, float value, const char* unit, uint16_t color);
  void drawStatus(int x, int y, const char* label, bool active, uint16_t colorActive);

public:
  SmartZone(int id, ZoneType type, PlantProfile* profile, ProfileThresholds* thresh);
  ~SmartZone();
  
  static void setTFT(TFT_eSPI* tftInstance); // Set shared TFT
  void begin();
  void run();
  void updateProfile(PlantProfile* profile, ProfileThresholds* thresh);
};

#endif
