#ifndef ZONE_A_H
#define ZONE_A_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <DHT.h>
#include <TFT_eSPI.h>

// Plant profile structure
struct PlantProfile {
  const char* name;
  // Air control parameters
  float tempMax;      // Nhiệt độ tối đa (°C)
  float tempMin;      // Nhiệt độ tối thiểu (°C)
  int humidityMin;    // Độ ẩm tối thiểu (%)
  int lightMin;       // Ánh sáng tối thiểu (%)
  // Soil control parameters
  int soilMoistureMin; // Độ ẩm đất tối thiểu (%)
  int nMin;            // Đạm tối thiểu (mg/kg)
  int pMin;            // Lân tối thiểu (mg/kg)
  int kMin;            // Kali tối thiểu (mg/kg)
};

// Predefined plant profiles
extern PlantProfile PROFILE_LETTUCE;
extern PlantProfile PROFILE_MELON;
extern PlantProfile PROFILE_STRAWBERRY;

class ZoneA {
private:
  // Hardware
  DHT* dht;
  Adafruit_NeoPixel* ringHeat;
  Adafruit_NeoPixel* ringMist;
  Adafruit_NeoPixel* ringGrow;
  // ringFan removed - using servo via PCA9685
  
  Adafruit_PWMServoDriver* pwm;  // Shared PCA9685 instance
  TFT_eSPI* tft;
  
  // Pins
  uint8_t pinDHT;
  uint8_t pinLDR;
  // pinRingFan removed - servo on PCA9685 ch4
  uint8_t pinRingHeat;
  uint8_t pinRingMist;
  uint8_t pinRingGrow;
  
  // Current profile
  PlantProfile* currentProfile;
  
  // Sensor readings
  float temperature;
  float humidity;
  int lightLevel;
  
  // Control states (with hysteresis)
  bool fanActive;
  bool heatActive;
  bool mistActive;
  bool growActive;
  
  // Simulation offsets for visual feedback
  float simTempOffset = 0.0;
  float simHumOffset = 0.0;
  
  // Previous raw sensor values (for change detection)
  float prevRawTemp = 25.0;
  float prevRawHum = 50.0;
  
  // Servo sweep animation
  int servoFanPos = 90;     // Current position (0-180)
  int servoFanDirection = 1; // 1 = forward, -1 = backward
  
  // Helper functions
  void updateSensors();
  void controlFan();
  void controlHeat();
  void controlMist();
  void controlGrowLight();
  void updateDisplay(int yOffset);
  
public:
  ZoneA(TFT_eSPI* display, Adafruit_PWMServoDriver* pwmDriver);
  ~ZoneA();
  
  void begin();
  void setProfile(PlantProfile* profile);
  void update(int displayYOffset = 0);
  
  // Getters
  float getTemperature() { return temperature; }
  float getHumidity() { return humidity; }
  int getLightLevel() { return lightLevel; }
};

#endif
