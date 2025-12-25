#ifndef ZONE_B_H
#define ZONE_B_H

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <TFT_eSPI.h>

// Forward declaration
struct PlantProfile;

class ZoneB {
private:
  // Hardware
  Adafruit_PWMServoDriver* pwm;  // Shared PCA9685 instance
  TFT_eSPI* tft;
  
  // Pins
  uint8_t pinPotMoist;
  uint8_t pinPotPH;
  uint8_t pinPotN;
  uint8_t pinPotP;
  uint8_t pinPotK;
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
  float simMoistOffset = 0.0;
  float simNOffset = 0.0;
  float simPOffset = 0.0;
  float simKOffset = 0.0;
  
  // Previous raw sensor values (for instant detection)
  int prevRawMoist = 50;
  int prevRawN = 300;
  int prevRawP = 200;
  int prevRawK = 250;
  
  // Servo sweep animations (PCA9685 channels 0-3)
  int servoWaterPos = 90;
  int servoWaterDir = 1;
  int servoNPos = 90;
  int servoNDir = 1;
  int servoPPos = 90;
  int servoPDir = 1;
  int servoKPos = 90;
  int servoKDir = 1;
  
  // Helper functions
  void updateSensors();
  void controlWater();
  void controlNutrients();
  void checkAlarm();
  void updateDisplay(int yOffset);
  
public:
  ZoneB(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver);
  ~ZoneB();
  
  void begin();
  void setProfile(PlantProfile* profile);
  void update(int displayYOffset = 0);
  
  // Getters
  int getSoilMoisture() { return soilMoisture; }
  float getSoilPH() { return soilPH; }
};

#endif
