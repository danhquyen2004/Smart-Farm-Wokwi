#include "ProfileManager.h"

ProfileManager profileManager;

ProfileManager::ProfileManager() {
  soLuongProfile = 0;
}

bool ProfileManager::begin() {
  // Mo namespace "farm"
  prefs.begin("farm", false);
  
  if (!loadProfiles()) {
    Serial.println("Chua co du lieu, tao mac dinh...");
    taoProfileMacDinh();
    saveProfiles();
  }
  
  Serial.printf("San sang với %d profiles\n", soLuongProfile);
  return true;
}

bool ProfileManager::loadProfiles() {
  // Doc chuoi JSON tu Preferences
  String json = prefs.getString("profiles", "");
  if (json == "") return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) return false;

  JsonArray array = doc.as<JsonArray>();
  soLuongProfile = 0;
  for (JsonObject obj : array) {
    if (soLuongProfile >= MAX_PROFILES) break;
    PlantProfile* p = &profiles[soLuongProfile];
    
    strlcpy(p->id, obj["id"] | "", sizeof(p->id));
    strlcpy(p->ten, obj["ten"] | "", sizeof(p->ten));
    p->nhietDoMax = obj["tMax"] | 28.0;
    p->nhietDoMin = obj["tMin"] | 15.0;
    p->doAmKKMin = obj["hMin"] | 60;
    p->anhSangMin = obj["lMin"] | 30;
    p->doAmDatMin = obj["mMin"] | 80;
    p->phMin = obj["phMi"] | 5.5;
    p->phMax = obj["phMa"] | 7.0;
    p->nMin = obj["n"] | 300;
    p->pMin = obj["p"] | 200;
    p->kMin = obj["k"] | 200;
    p->ecMin = obj["eMi"] | 1.2;
    p->ecMax = obj["eMa"] | 2.0;
    soLuongProfile++;
  }
  return soLuongProfile > 0;
}

bool ProfileManager::saveProfiles() {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < soLuongProfile; i++) {
    PlantProfile* p = &profiles[i];
    JsonObject obj = array.add<JsonObject>();
    obj["id"] = p->id;
    obj["ten"] = p->ten;
    obj["tMax"] = p->nhietDoMax;
    obj["tMin"] = p->nhietDoMin;
    obj["hMin"] = p->doAmKKMin;
    obj["lMin"] = p->anhSangMin;
    obj["mMin"] = p->doAmDatMin;
    obj["phMi"] = p->phMin;
    obj["phMa"] = p->phMax;
    obj["n"] = p->nMin;
    obj["p"] = p->pMin;
    obj["k"] = p->kMin;
    obj["eMi"] = p->ecMin;
    obj["eMa"] = p->ecMax;
  }

  String json;
  serializeJson(doc, json);
  return prefs.putString("profiles", json) > 0;
}

PlantProfile* ProfileManager::getProfile(int index) {
  if (index < 0 || index >= soLuongProfile) return nullptr;
  return &profiles[index];
}

PlantProfile* ProfileManager::getProfileById(const char* id) {
  for (int i = 0; i < soLuongProfile; i++) {
    if (strcmp(profiles[i].id, id) == 0) return &profiles[i];
  }
  return nullptr;
}

bool ProfileManager::themProfile(PlantProfile* profile) {
  if (soLuongProfile >= MAX_PROFILES) return false;
  memcpy(&profiles[soLuongProfile], profile, sizeof(PlantProfile));
  soLuongProfile++;
  return saveProfiles();
}

bool ProfileManager::capNhatProfile(const char* id, PlantProfile* profile) {
  PlantProfile* p = getProfileById(id);
  if (!p) return false;
  memcpy(p, profile, sizeof(PlantProfile));
  strlcpy(p->id, id, sizeof(p->id)); // Giu nguyên id
  return saveProfiles();
}

bool ProfileManager::xoaProfile(const char* id) {
  int idx = -1;
  for (int i = 0; i < soLuongProfile; i++) {
    if (strcmp(profiles[i].id, id) == 0) { idx = i; break; }
  }
  if (idx == -1) return false;
  for (int i = idx; i < soLuongProfile - 1; i++) profiles[i] = profiles[i+1];
  soLuongProfile--;
  return saveProfiles();
}

void ProfileManager::taoProfileMacDinh() {
  soLuongProfile = 0;
  PlantProfile p;
  
  // Xa Lach
  strlcpy(p.id, "xa_lach", 20); strlcpy(p.ten, "XA LACH", 30);
  p.nhietDoMax = 28.0; p.nhietDoMin = 15.0; p.doAmKKMin = 60; p.anhSangMin = 30;
  p.doAmDatMin = 80; p.phMin = 5.5; p.phMax = 7.0; p.nMin = 300; p.pMin = 200; p.kMin = 200;
  p.ecMin = 1.2; p.ecMax = 2.0;
  themProfile(&p);

  // Dau Tay
  strlcpy(p.id, "dau_tay", 20); strlcpy(p.ten, "DAU TAY", 30);
  p.nhietDoMax = 24.0; p.nhietDoMin = 12.0; p.doAmKKMin = 65; p.anhSangMin = 35;
  p.doAmDatMin = 65; p.phMin = 5.5; p.phMax = 6.5; p.nMin = 250; p.pMin = 250; p.kMin = 300;
  p.ecMin = 1.0; p.ecMax = 1.8;
  themProfile(&p);
}
