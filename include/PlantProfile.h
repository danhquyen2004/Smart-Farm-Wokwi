#ifndef PLANTPROFILE_H
#define PLANTPROFILE_H

// Plant profile types
enum PlantProfile {
  PROFILE_LETTUCE,    // Xà lách: Cool, high moisture, low EC
  PROFILE_MELON,      // Dưa lưới: Hot, dry, high K, high EC tolerance
  PROFILE_STRAWBERRY  // Dâu tây: Moderate, balanced NPK
};

// Zone types
enum ZoneType {
  ZONE_AIR,    // Zone A: Air environment control
  ZONE_SOIL,   // Zone B: Soil/Precision agriculture
  ZONE_HYDRO   // Zone C: Hydroponic system
};

// Threshold structure for each profile
struct ProfileThresholds {
  // Air control thresholds
  int tempMax;         // Maximum temperature before fan activates (°C)
  int tempMin;         // Minimum temperature for heater (°C)
  int humidityMin;     // Minimum humidity for mist (%)
  int lightMin;        // Minimum light for grow light (%)
  
  // Soil control thresholds
  int soilMoistureMin; // Minimum soil moisture (%)
  int nMin;            // Minimum Nitrogen (mg/kg)
  int pMin;            // Minimum Phosphorus (mg/kg)
  int kMin;            // Minimum Potassium (mg/kg)
  float ecMax;         // Maximum EC - safety limit (mS/cm)
  float phMin;         // Minimum pH
  float phMax;         // Maximum pH
  
  // Hydro control thresholds
  int waterTempMax;    // Maximum water temperature (°C)
  float hydroEcMin;    // Minimum EC for nutrient solution (mS/cm)
  float hydroPhMin;    // Minimum pH for hydro
  float hydroPhMax;    // Maximum pH for hydro
};

// Profile names for display
inline const char* getProfileName(PlantProfile profile) {
  switch(profile) {
    case PROFILE_LETTUCE:    return "LETTUCE";
    case PROFILE_MELON:      return "MELON";
    case PROFILE_STRAWBERRY: return "STRAWBERRY";
    default:                 return "UNKNOWN";
  }
}

// Get zone name for display
inline const char* getZoneName(ZoneType zone) {
  switch(zone) {
    case ZONE_AIR:   return "AIR";
    case ZONE_SOIL:  return "SOIL";
    case ZONE_HYDRO: return "HYDRO";
    default:         return "UNKNOWN";
  }
}

// Load default thresholds for a profile
inline void loadProfileThresholds(PlantProfile profile, ProfileThresholds& thresholds) {
  switch(profile) {
    case PROFILE_LETTUCE:
      // Lettuce: Cool, moist, low nutrients
      thresholds.tempMax = 28;
      thresholds.tempMin = 15;
      thresholds.humidityMin = 60;
      thresholds.lightMin = 30;
      
      thresholds.soilMoistureMin = 80;
      thresholds.nMin = 300;  // High N for leafy greens
      thresholds.pMin = 200;
      thresholds.kMin = 200;
      thresholds.ecMax = 1.2;
      thresholds.phMin = 6.0;
      thresholds.phMax = 7.0;
      
      thresholds.waterTempMax = 22;
      thresholds.hydroEcMin = 0.8;
      thresholds.hydroPhMin = 5.5;
      thresholds.hydroPhMax = 6.5;
      break;
      
    case PROFILE_MELON:
      // Melon: Hot, dry, high K
      thresholds.tempMax = 35;
      thresholds.tempMin = 18;
      thresholds.humidityMin = 50;
      thresholds.lightMin = 40;
      
      thresholds.soilMoistureMin = 50;
      thresholds.nMin = 200;
      thresholds.pMin = 250;
      thresholds.kMin = 400;  // High K for sweetness
      thresholds.ecMax = 2.5;
      thresholds.phMin = 6.0;
      thresholds.phMax = 7.5;
      
      thresholds.waterTempMax = 28;
      thresholds.hydroEcMin = 1.5;
      thresholds.hydroPhMin = 6.0;
      thresholds.hydroPhMax = 7.0;
      break;
      
    case PROFILE_STRAWBERRY:
      // Strawberry: Moderate, balanced
      thresholds.tempMax = 24;
      thresholds.tempMin = 12;
      thresholds.humidityMin = 65;
      thresholds.lightMin = 35;
      
      thresholds.soilMoistureMin = 65;
      thresholds.nMin = 250;
      thresholds.pMin = 250;
      thresholds.kMin = 300;
      thresholds.ecMax = 1.8;
      thresholds.phMin = 5.5;
      thresholds.phMax = 6.5;
      
      thresholds.waterTempMax = 20;
      thresholds.hydroEcMin = 1.0;
      thresholds.hydroPhMin = 5.5;
      thresholds.hydroPhMax = 6.5;
      break;
  }
}

#endif
