#ifndef PLANT_ZONE_H
#define PLANT_ZONE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <DHT.h>
#include <WiFi.h>
#include "ProfileManager.h"


/**
 * PlantZone - Quan ly mot vung trong
 * Bao gom: Cam bien, Thiet bi dieu khien
 */
class PlantZone {
private:
  // Zone ID (0 = Vung 1, 1 = Vung 2, ...)
  uint8_t maVung;
  
  // Thiet bi chia se
  TFT_eSPI* tft;
  Adafruit_PWMServoDriver* pwm;
  
  // Cam bien moi truong chia se (static)
  static DHT* sharedDHT;
  
  // Thiet bi rieng cua vung
  DHT* dht;
  Adafruit_NeoPixel* vongLed;
  
  // Chan GPIO
  uint8_t pinDHT;
  uint8_t pinLDR;
  uint8_t pinChuong;
  uint8_t pinVongLed;
  
  // Kenh PCA9685
  uint8_t kenhQuat;
  uint8_t kenhBomNuoc;
  uint8_t kenhBomN;
  uint8_t kenhBomP;
  uint8_t kenhBomK;
  uint8_t kenhBomDinhDuong;
  
  // Cau hinh cay hien tai
  PlantProfile* cauHinh;
  
  // Gia tri cam bien
  float nhietDo;
  float doAmKK;
  int anhSang;
  int doAmDat;
  float phDat;
  int n;
  int p;
  int k;
  float ec;
  
  // Trang thai thiet bi
  bool quatChay;
  bool suoiChay;
  bool phunSuongChay;
  bool denChay;
  bool bomNuocChay;
  bool bomNChay;
  bool bomPChay;
  bool bomKChay;
  bool bomDinhDuongChay;
  bool canhBao;
  
  // Offset mo phong (physics simulation)
  float offsetNhietDo;
  float offsetDoAmKK;
  float offsetDoAmDat;
  float offsetN;
  float offsetP;
  float offsetK;
  float offsetEC;
  
  // Gia tri raw truoc do (de phat hien thay doi)
  float rawNhietDoTruoc;
  float rawDoAmKKTruoc;
  int rawDoAmDatTruoc;
  int rawNTruoc;
  int rawPTruoc;
  int rawKTruoc;
  int rawECTruoc;
  
  // Trang thai servo animation
  int viTriServoQuat, huongServoQuat;
  int viTriServoBomNuoc, huongServoBomNuoc;
  int viTriServoBomN, huongServoBomN;
  int viTriServoBomP, huongServoBomP;
  int viTriServoBomK, huongServoBomK;
  int viTriServoBomDinhDuong, huongServoBomDinhDuong;
  
  // Timer toi uu hoa
  unsigned long lanCapNhatTFT;
  unsigned long lanDocDHT;
  
  // Gia tri hien thi truoc do (de phat hien thay doi)
  float hienThiNhietDoTruoc;
  float hienThiDoAmKKTruoc;
  int hienThiAnhSangTruoc;
  int hienThiDoAmDatTruoc;
  float hienThiPHTruoc;
  int hienThiNTruoc, hienThiPTruoc, hienThiKTruoc;
  float hienThiECTruoc;
  bool hienThiCanhBaoTruoc;
  
  // Che do Tu dong/Thu cong
  bool cheTuDong;
  
  // Phuong thuc helper
  void docCamBien();
  void moPhongVatLy();
  void dieuKhienThietBi();
  void capNhatManHinh(int yOffset);
  
  // Dieu khien tung thiet bi
  void dieuKhienQuat();
  void dieuKhienSuoi();
  void dieuKhienPhunSuong();
  void dieuKhienDen();
  void dieuKhienBomNuoc();
  void dieuKhienBomN();
  void dieuKhienBomP();
  void dieuKhienBomK();
  void dieuKhienBomDinhDuong();
  void kiemTraCanhBao();
  void capNhatChuong();
  
  // Phuong thuc helper tai su dung
  void animateServo(bool dangChay, int& viTri, int& huong, uint8_t kenh);
  void datMauVongLed(int pixelDau, int pixelCuoi, bool dangChay, uint8_t r, uint8_t g, uint8_t b);

public:
  PlantZone(uint8_t id, TFT_eSPI* display, Adafruit_PWMServoDriver* servo);
  ~PlantZone();
  
  void begin();
  void update(int displayYOffset = 0);
  
  // Quan ly cau hinh cay
  void setCauHinh(PlantProfile* cauHinhMoi);
  PlantProfile* getCauHinh() { return cauHinh; }
  
  // Dieu khien che do
  void setCheTuDong(bool batTuDong);
  bool getCheTuDong() { return cheTuDong; }
  
  // Dieu khien thu cong (khi khong o che do tu dong)
  void batTatQuat(bool bat);
  void batTatSuoi(bool bat);
  void batTatPhunSuong(bool bat);
  void batTatDen(bool bat);
  void batTatBomNuoc(bool bat);
  void batTatBomN(bool bat);
  void batTatBomP(bool bat);
  void batTatBomK(bool bat);
  void batTatBomDinhDuong(bool bat);
  
  // Lay gia tri cam bien
  float getNhietDo() { return nhietDo; }
  float getDoAmKK() { return doAmKK; }
  int getAnhSang() { return anhSang; }
  int getDoAmDat() { return doAmDat; }
  float getPHDat() { return phDat; }
  int getN() { return n; }
  int getP() { return p; }
  int getK() { return k; }
  float getEC() { return ec; }
  
  // Lay trang thai thiet bi
  bool laQuatChay() { return quatChay; }
  bool laSuoiChay() { return suoiChay; }
  bool laPhunSuongChay() { return phunSuongChay; }
  bool laDenChay() { return denChay; }
  bool laBomNuocChay() { return bomNuocChay; }
  bool laBomNChay() { return bomNChay; }
  bool laBomPChay() { return bomPChay; }
  bool laBomKChay() { return bomKChay; }
  bool laBomDinhDuongChay() { return bomDinhDuongChay; }
  bool laCanhBao() { return canhBao; }
  
  // Backward compatibility (for MenuSystem)
  bool getAutoMode() { return cheTuDong; }
  void setAutoMode(bool enabled) { cheTuDong = enabled; }
  PlantProfile* getProfile() { return cauHinh; }
  bool isAlarmActive() { return canhBao; }
};

#endif // PLANT_ZONE_H

