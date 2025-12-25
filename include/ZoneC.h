#ifndef ZONE_C_H
#define ZONE_C_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Forward declaration
struct PlantProfile;

class ZoneC {
private:
  // Hardware
  OneWire* oneWire;
  DallasTemperature* ds18b20;
  Adafruit_NeoPixel* ringChiller;
  Adafruit_NeoPixel* ringNutrient;
  TFT_eSPI* tft;
  
  // Pins
  uint8_t pinDS18B20;
  uint8_t pinPotEC;
  uint8_t pinPotPH;
  uint8_t pinRingChiller;
  uint8_t pinRingNutrient;
  
  // Current profile
  PlantProfile* currentProfile;
  
  // Sensor readings
  float waterTemp;
  float hydroEC;
  float hydroPH;
  
  // Control states
  bool chillerActive;
  bool nutrientActive;
  
  // Simulation offsets
  float simTempOffset;
  float simECOffset;
  
  // Helper functions
  void updateSensors();
  void controlChiller();
  void controlNutrient();
  void updateDisplay(int yOffset);
  
public:
  ZoneC(TFT_eSPI* display);
  ~ZoneC();
  
  void begin();
  void setProfile(PlantProfile* profile);
  void update(int displayYOffset = 0);
  
  // Getters
  float getWaterTemp() { return waterTemp; }
  float getHydroEC() { return hydroEC; }
  float getHydroPH() { return hydroPH; }
};

#endif
