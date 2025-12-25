#ifndef ZONE_B_H
#define ZONE_B_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>

// Forward declaration
struct PlantProfile;

class ZoneB {
private:
  // Hardware
  Adafruit_NeoPixel* ringWater;
  Adafruit_NeoPixel* ringN;
  Adafruit_NeoPixel* ringP;
  Adafruit_NeoPixel* ringK;
  TFT_eSPI* tft;
  
  // Pins
  uint8_t pinPotMoist;
  uint8_t pinPotPH;
  uint8_t pinPotN;
  uint8_t pinPotP;
  uint8_t pinPotK;
  uint8_t pinRingWater;
  uint8_t pinRingN;
  uint8_t pinRingP;
  uint8_t pinRingK;
  uint8_t pinBuzzer;
  
  // Current profile
  PlantProfile* currentProfile;
  
  // Sensor readings
  int soilMoisture;
  float soilPH;
  int nitrogenLevel;
  int phosphorusLevel;
  int potassiumLevel;
  
  // Control states
  bool waterActive;
  bool nActive;
  bool pActive;
  bool kActive;
  bool alarmActive;
  
  // Simulation offsets
  float simMoistOffset;
  float simNOffset;
  float simPOffset;
  float simKOffset;
  
  // Helper functions
  void updateSensors();
  void controlWater();
  void controlNutrients();
  void checkAlarm();
  void updateDisplay(int yOffset);
  
public:
  ZoneB(TFT_eSPI* display);
  ~ZoneB();
  
  void begin();
  void setProfile(PlantProfile* profile);
  void update(int displayYOffset = 0);
  
  // Getters
  int getSoilMoisture() { return soilMoisture; }
  float getSoilPH() { return soilPH; }
};

#endif
