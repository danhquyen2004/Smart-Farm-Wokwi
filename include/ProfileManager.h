#ifndef PROFILE_MANAGER_H
#define PROFILE_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define MAX_PROFILES 10

struct PlantProfile {
  char id[20];
  char ten[30];
  float nhietDoMax;
  float nhietDoMin;
  int doAmKKMin;
  int anhSangMin;
  int doAmDatMin;
  float phMin;
  float phMax;
  int nMin;
  int pMin;
  int kMin;
  float ecMin;
  float ecMax;
};

class ProfileManager {
private:
  PlantProfile profiles[MAX_PROFILES];
  int soLuongProfile;
  Preferences prefs;
  
public:
  ProfileManager();
  bool begin();
  bool loadProfiles();
  bool saveProfiles();
  
  int getSoLuongProfile() { return soLuongProfile; }
  PlantProfile* getProfile(int index);
  PlantProfile* getProfileById(const char* id);
  bool themProfile(PlantProfile* profile);
  bool capNhatProfile(const char* id, PlantProfile* profile);
  bool xoaProfile(const char* id);
  void taoProfileMacDinh();
};

extern ProfileManager profileManager;

#endif
