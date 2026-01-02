#ifndef PLANT_ZONE_H
#define PLANT_ZONE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <DHT.h>
#include "MuxManager.h"

/**
 * PlantProfile - Configuration for a specific plant type
 */
struct PlantProfile {
  const char* name;
  
  // Air thresholds
  float tempMax;          // Fan ON if above
  float tempMin;          // Heat ON if below
  int humidityMin;        // Mist ON if below
  int lightMin;           // Grow light ON if below
  
  // Soil thresholds
  int moistureMin;        // Water pump ON if below
  float phMin;            // Alarm if below
  float phMax;            // Alarm if above
  int nitrogenMin;        // N pump ON if below
  int phosphorusMin;      // P pump ON if below
  int potassiumMin;       // K pump ON if below
  
  // Hydro thresholds
  float ecMin;            // Nutrient pump ON if below
  float ecMax;            // Nutrient pump OFF if above
};

/**
 * PlantZone - Complete management of one growing zone
 * Includes: Air sensors, Soil sensors, Hydro sensors, All actuators
 */
class PlantZone {
private:
  // Zone ID (0 = Zone 1, 1 = Zone 2, etc.)
  uint8_t zoneId;
  
  // Shared hardware references
  TFT_eSPI* tft;
  Adafruit_PWMServoDriver* pwm;
  MuxManager* mux;
  
  // Shared environment sensors (static, shared across all zones)
  static DHT* sharedDHT;
  
  // Zone-specific hardware
  DHT* dht;  // Points to sharedDHT for all zones
  Adafruit_NeoPixel* ledRing; // Handling daisy-chained Heat, Mist, Grow (36 pixels total)
  
  // Pin assignments
  uint8_t pinDHT;
  uint8_t pinLDR;
  uint8_t pinBuzzer;  // Zone-specific buzzer
  uint8_t pinLedRing;
  
  // PCA9685 channels
  uint8_t pwmChFan;
  uint8_t pwmChWater;
  uint8_t pwmChN;
  uint8_t pwmChP;
  uint8_t pwmChK;
  uint8_t pwmChNutrient;
  
  // Current plant profile
  PlantProfile* profile;
  
  // Sensor readings
  float temperature;
  float humidity;
  int lightLevel;
  int moisture;
  float soilPH;
  int nitrogen;
  int phosphorus;
  int potassium;
  float hydroEC;
  
  // Actuator states
  bool fanActive;
  bool heatActive;
  bool mistActive;
  bool growActive;
  bool waterActive;
  bool nActive;
  bool pActive;
  bool kActive;
  bool nutrientActive;
  bool alarmActive;
  
  // Simulation offsets (for physics simulation)
  float simTempOffset;
  float simHumOffset;
  float simMoistOffset;
  float simNOffset;
  float simPOffset;
  float simKOffset;
  float simECOffset;
  
  // Previous raw values (for manual change detection)
  float prevRawTemp;
  float prevRawHum;
  int prevRawMoist;
  int prevRawN;
  int prevRawP;
  int prevRawK;
  int prevRawEC;
  
  // Servo animation states
  int servoFanPos, servoFanDir;
  int servoWaterPos, servoWaterDir;
  int servoNPos, servoNDir;
  int servoPPos, servoPDir;
  int servoKPos, servoKDir;
  int servoNutrientPos, servoNutrientDir;
  
  // Performance optimization timers
  unsigned long lastUpdateTFT;
  unsigned long lastReadDHT;
  
  // Stored values for change detection
  float lastDispTemp;
  float lastDispHum;
  int lastDispLight;
  int lastDispMoist;
  float lastDispPH;
  int lastDispN, lastDispP, lastDispK;
  float lastDispEC;
  bool lastDispAlarm;
  
  // Auto/Manual mode
  bool isAutoMode;
  
  // Helper methods
  void readSensors();
  void controlActuators();
  void updateDisplay(int yOffset);
  
  // Individual actuator controls
  void controlFan();
  void controlHeat();
  void controlMist();
  void controlGrow();
  void controlWater();
  void controlN();
  void controlP();
  void controlK();
  void controlNutrient();
  void checkAlarm();
  void updateBuzzer();  // Zone-specific buzzer control
  
public:
  PlantZone(uint8_t id, TFT_eSPI* display, Adafruit_PWMServoDriver* servo, MuxManager* multiplexer);
  ~PlantZone();
  
  void begin();
  void update(int displayYOffset = 0);
  
  // Profile management
  void setProfile(PlantProfile* newProfile);
  PlantProfile* getProfile() { return profile; }
  
  // Mode control
  void setAutoMode(bool enabled) { isAutoMode = enabled; }
  bool getAutoMode() { return isAutoMode; }
  
  // Manual actuator control (when not in auto mode)
  void setFan(bool on);
  void setHeat(bool on);
  void setMist(bool on);
  void setGrow(bool on);
  void setWater(bool on);
  void setNutrient(bool on);
  
  // Getters for sensor values
  float getTemperature() { return temperature; }
  float getHumidity() { return humidity; }
  int getLightLevel() { return lightLevel; }
  int getMoisture() { return moisture; }
  float getSoilPH() { return soilPH; }
  int getNitrogen() { return nitrogen; }
  int getPhosphorus() { return phosphorus; }
  int getPotassium() { return potassium; }
  float getHydroEC() { return hydroEC; }
  
  // Getters for actuator states
  bool isFanActive() { return fanActive; }
  bool isHeatActive() { return heatActive; }
  bool isMistActive() { return mistActive; }
  bool isGrowActive() { return growActive; }
  bool isWaterActive() { return waterActive; }
  bool isNutrientActive() { return nutrientActive; }
  bool isAlarmActive() { return alarmActive; }
};

// Predefined plant profiles
extern PlantProfile PROFILE_LETTUCE;
extern PlantProfile PROFILE_STRAWBERRY;
extern PlantProfile PROFILE_TOMATO;

#endif // PLANT_ZONE_H
