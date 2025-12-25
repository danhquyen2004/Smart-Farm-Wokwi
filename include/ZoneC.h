#ifndef ZONE_C_H
#define ZONE_C_H

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <TFT_eSPI.h>

// Forward declaration
struct PlantProfile;

class ZoneC {
private:
  // Hardware
  Adafruit_PWMServoDriver* pwm;  // Shared PCA9685
  TFT_eSPI* tft;
  
  // Pins
  uint8_t pinPotEC;
  uint8_t pinPotPH;  // Only EC and pH (no temp)
  
  // Current profile
  PlantProfile* currentProfile;
  
  // Sensor readings
  float hydroEC;
  float hydroPH;  // No water temp
  
  // Control states
  bool nutrientActive;  // Only nutrient pump (no chiller)
  
  // Simulation offsets
  float simECOffset = 0.0;  // Only EC simulation
  
  // Previous raw sensor values (for instant detection)
  int prevRawEC = 1500;  // Only EC tracking
  
  // Servo sweep animation (PCA9685 channel 6 only)
  int servoNutrientPos = 90;
  int servoNutrientDir = 1;  // No chiller servo
  
  // Helper functions
  void updateSensors();
  void controlNutrient();  // Only nutrient (no chiller)
  void updateDisplay(int yOffset);
  
public:
  ZoneC(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver);
  ~ZoneC();
  
  void begin();
  void setProfile(PlantProfile* profile);
  void update(int displayYOffset = 0);
  
  // Getters
  float getHydroEC() { return hydroEC; }
  float getHydroPH() { return hydroPH; }  // No water temp getter
};

#endif
