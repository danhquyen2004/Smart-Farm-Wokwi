#ifndef MUX_MANAGER_H
#define MUX_MANAGER_H

#include <Arduino.h>

/**
 * MuxManager - Manages reading sensors via CD74HC4067 multiplexer
 * 
 * ESP32 DevKit V1 - Shared Environment Edition
 * All sensors are shared between both zones
 * 
 * Pin Configuration:
 *   S0 -> GPIO 16
 *   S1 -> GPIO 17
 *   S2 -> GPIO 5
 *   S3 -> GPIO 4
 *   EN -> GND (always enabled)
 * 
 * MUX1 (Shared Analog Sensors):
 *   COM (SIG) -> GPIO 34 (ADC1 - WiFi Safe)
 *   Channel Mapping:
 *     Shared (0-5): Moisture, pH, N, P, K, EC
 *     Reserved (6-15): Available for expansion
 */

// Mux channel definitions for shared analog sensors (MUX1)
enum MuxChannel {
  // Shared environment sensors (used by both zones)
  MUX_MOISTURE = 0,
  MUX_PH_SOIL = 1,
  MUX_NITROGEN = 2,
  MUX_PHOSPHORUS = 3,
  MUX_POTASSIUM = 4,
  MUX_EC = 5,
  
  // Reserved channels (6-15 available for expansion)
  MUX_RESERVED_6 = 6,
  MUX_RESERVED_7 = 7,
  MUX_RESERVED_8 = 8,
  MUX_RESERVED_9 = 9,
  MUX_RESERVED_10 = 10,
  MUX_RESERVED_11 = 11,
  MUX_RESERVED_12 = 12,
  MUX_RESERVED_13 = 13,
  MUX_RESERVED_14 = 14,
  MUX_RESERVED_15 = 15
};

class MuxManager {
private:
  // ESP32 DevKit V1 GPIO pins for MUX control
  static const uint8_t PIN_MUX_S0 = 16;
  static const uint8_t PIN_MUX_S1 = 17;
  static const uint8_t PIN_MUX_S2 = 26; // Moved from 5 to avoid SPI SS conflict
  static const uint8_t PIN_MUX_S3 = 14; // Moved from 4 to avoid SPI conflict
  
  // MUX1: Analog sensors
  static const uint8_t PIN_MUX1_COM = 34;  // ADC1 - WiFi Safe
  
  // Settling time after channel switch (microseconds)
  static const uint16_t SETTLING_TIME_US = 200; // Increased for stability
  
  // Select a channel on the multiplexers (0-15)
  // Both MUXes will select the same channel due to shared S0-S3
  void selectChannel(uint8_t channel);
  
public:
  MuxManager();
  
  // Initialize pins
  void begin();
  
  // ==================== MUX1 (Analog Sensors) ====================
  
  // Read raw ADC value from MUX1 channel (0-4095)
  int readRaw(uint8_t channel);
  
  // Read and map to percentage (0-100)
  int readPercent(uint8_t channel);
  
  // Read and map to custom range
  float readMapped(uint8_t channel, float minVal, float maxVal);
  
  // Convenience methods for specific sensors
  int readMoisture();       // 0-100%
  float readSoilPH();       // 0.0-14.0
  int readNitrogen();       // 0-500 mg/kg
  int readPhosphorus();     // 0-500 mg/kg
  int readPotassium();      // 0-500 mg/kg
  float readEC();           // 0.00-3.00 mS/cm
};

#endif // MUX_MANAGER_H
