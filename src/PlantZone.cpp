#include "PlantZone.h"

// ==================== HANG SO MO PHONG ====================

// Toc do thay doi nhiet do (°C moi chu ky)
const float TOC_DO_LAM_MAT = 0.15f;
const float TOC_DO_LAM_NONG = 0.15f;
const float TOC_DO_PHUC_HOI_NHIET = 0.05f;

// Toc do thay doi do am (% moi chu ky)
const float TOC_DO_TANG_DO_AM = 0.3f;
const float TOC_DO_GIAM_DO_AM = 0.1f;

// Toc do thay doi do am dat (% moi chu ky)
const float TOC_DO_TANG_DO_AM_DAT = 0.5f;
const float TOC_DO_GIAM_DO_AM_DAT = 0.1f;

// Toc do thay doi dinh duong (mg/kg moi chu ky)
const float TOC_DO_TANG_DINH_DUONG = 0.8f;
const float TOC_DO_GIAM_DINH_DUONG = 0.2f;

// Toc do thay doi EC (mS/cm moi chu ky)
const float TOC_DO_TANG_EC = 0.05f;
const float TOC_DO_GIAM_EC = 0.01f;

// Nguong tre (Hysteresis)
const float TRE_NHIET_DO = 2.0f;
const float TRE_DO_AM_KK = 5.0f;
const int TRE_ANH_SANG = 10;
const int TRE_DO_AM_DAT = 10;
const int TRE_DINH_DUONG = 30;
const float TRE_EC = 0.2f;

// ==================== CAU HINH CAY ====================

PlantProfile CAU_HINH_XA_LACH = {
  "XA LACH",
  28.0f, 15.0f, 60, 30,           // Khong khi: nhietDoMax, nhietDoMin, doAmKKMin, anhSangMin
  80, 5.5f, 7.0f, 300, 200, 200,  // Dat: doAmDatMin, phMin, phMax, nMin, pMin, kMin
  1.2f, 2.0f                       // Thuy canh: ecMin, ecMax
};

PlantProfile CAU_HINH_DAU_TAY = {
  "DAU TAY",
  24.0f, 12.0f, 65, 35,
  65, 5.5f, 6.5f, 250, 250, 300,
  1.0f, 1.8f
};

PlantProfile CAU_HINH_CA_CHUA = {
  "CA CHUA",
  30.0f, 18.0f, 55, 40,
  70, 6.0f, 7.0f, 200, 250, 350,
  2.0f, 3.0f
};

// ==================== STATIC MEMBERS ====================

DHT* PlantZone::sharedDHT = nullptr;

// ==================== CONSTRUCTOR / DESTRUCTOR ====================

PlantZone::PlantZone(uint8_t id, TFT_eSPI* display, Adafruit_PWMServoDriver* servo) {
  maVung = id;
  tft = display;
  pwm = servo;
  
  dht = nullptr;
  vongLed = nullptr;
  
  // ESP32 DevKit V1 - Chia se moi truong
  // Tat ca cac vung dung chung cam bien moi truong
  
  // Chan cam bien chia se (Vung 0 khoi tao, Vung 1 doc cung gia tri)
  pinDHT = 25;      // DHT22 chia se
  pinLDR = 36;      // LDR chia se (VP)
  
  // Chan thiet bi rieng cua tung vung
  if (maVung == 0) {
    pinChuong = 4;
    pinVongLed = 12;
  } else {
    pinChuong = 5;
    pinVongLed = 13;
  }
  
  // Kenh PCA9685
  kenhQuat = maVung * 6;
  kenhBomNuoc = maVung * 6 + 1;
  kenhBomN = maVung * 6 + 2;
  kenhBomP = maVung * 6 + 3;
  kenhBomK = maVung * 6 + 4;
  kenhBomDinhDuong = maVung * 6 + 5;
  
  // Khoi tao gia tri cam bien
  nhietDo = 25.0f;
  doAmKK = 50.0f;
  anhSang = 50;
  doAmDat = 50;
  phDat = 6.5f;
  n = 300;
  p = 200;
  k = 250;
  ec = 1.5f;
  
  // Khoi tao trang thai thiet bi
  quatChay = false;
  suoiChay = false;
  phunSuongChay = false;
  denChay = false;
  bomNuocChay = false;
  bomNChay = false;
  bomPChay = false;
  bomKChay = false;
  bomDinhDuongChay = false;
  canhBao = false;
  
  // Khoi tao offset mo phong
  offsetNhietDo = 0;
  offsetDoAmKK = 0;
  offsetDoAmDat = 0;
  offsetN = 0;
  offsetP = 0;
  offsetK = 0;
  offsetEC = 0;
  
  // Khoi tao gia tri raw truoc do
  rawNhietDoTruoc = 25.0f;
  rawDoAmKKTruoc = 50.0f;
  rawDoAmDatTruoc = 2048;
  rawNTruoc = 2048;
  rawPTruoc = 2048;
  rawKTruoc = 2048;
  rawECTruoc = 2048;
  
  // Khoi tao trang thai servo
  viTriServoQuat = 90; huongServoQuat = 1;
  viTriServoBomNuoc = 90; huongServoBomNuoc = 1;
  viTriServoBomN = 90; huongServoBomN = 1;
  viTriServoBomP = 90; huongServoBomP = 1;
  viTriServoBomK = 90; huongServoBomK = 1;
  viTriServoBomDinhDuong = 90; huongServoBomDinhDuong = 1;
  
  // Mac dinh che do tu dong
  cheTuDong = true;
  
  // Cau hinh mac dinh
  cauHinh = &CAU_HINH_XA_LACH;
  
  // Toi uu hoa hieu suat
  lanCapNhatTFT = 0;
  lanDocDHT = 0;
  hienThiNhietDoTruoc = -999;
  hienThiDoAmKKTruoc = -999;
  hienThiAnhSangTruoc = -999;
  hienThiDoAmDatTruoc = -999;
  hienThiPHTruoc = -999;
  hienThiNTruoc = -999; hienThiPTruoc = -999; hienThiKTruoc = -999;
  hienThiECTruoc = -999;
  hienThiCanhBaoTruoc = false;
}

PlantZone::~PlantZone() {
  if (dht) delete dht;
  if (vongLed) delete vongLed;
}

void PlantZone::begin() {
  // Khoi tao DHT22 chia se (chi 1 lan cho tat ca vung)
  if (sharedDHT == nullptr) {
    sharedDHT = new DHT(pinDHT, DHT22);
    sharedDHT->begin();
    Serial.println("[Chia se] DHT22 da khoi tao");
  }
  dht = sharedDHT;
  
  // Khoi tao vong LED daisy-chain
  vongLed = new Adafruit_NeoPixel(36, pinVongLed, NEO_GRB + NEO_KHZ800);
  vongLed->begin();
  delay(50);
  vongLed->clear(); 
  vongLed->show();
  delay(50);
  
  // Khoi tao chan LDR
  pinMode(pinLDR, INPUT);
  
  // Khoi tao chan chuong
  pinMode(pinChuong, OUTPUT);
  digitalWrite(pinChuong, LOW);
  
  // Dung tat ca servo o vi tri giua
  pwm->setPWM(kenhQuat, 0, 375);
  pwm->setPWM(kenhBomNuoc, 0, 375);
  pwm->setPWM(kenhBomN, 0, 375);
  pwm->setPWM(kenhBomP, 0, 375);
  pwm->setPWM(kenhBomK, 0, 375);
  pwm->setPWM(kenhBomDinhDuong, 0, 375);
  
  Serial.printf("PlantZone %d: Da khoi tao (Moi truong chia se)\n", maVung + 1);
}

void PlantZone::setCauHinh(PlantProfile* cauHinhMoi) {
  cauHinh = cauHinhMoi;
  Serial.printf("Vung %d: Cau hinh thay doi thanh %s\n", maVung + 1, cauHinh->ten);
}

// ==================== DOC CAM BIEN ====================

void PlantZone::docCamBien() {
  // Doc DHT22 moi 2 giay (toi uu hoa)
  if (millis() - lanDocDHT > 2000) {
    float rawNhietDo = dht->readTemperature();
    float rawDoAmKK = dht->readHumidity();
    
    if (!isnan(rawNhietDo)) {
      float deltaTemp = abs(rawNhietDo - rawNhietDoTruoc);
      if (deltaTemp > 2.0f) {
        offsetNhietDo = 0;
        nhietDo = rawNhietDo;
      } else {
        nhietDo = rawNhietDo + offsetNhietDo;
      }
      rawNhietDoTruoc = rawNhietDo;
    }
    
    if (!isnan(rawDoAmKK)) {
      float deltaHum = abs(rawDoAmKK - rawDoAmKKTruoc);
      if (deltaHum > 5.0f) {
        offsetDoAmKK = 0;
        doAmKK = rawDoAmKK;
      } else {
        doAmKK = rawDoAmKK + offsetDoAmKK;
      }
      rawDoAmKKTruoc = rawDoAmKK;
    }
    lanDocDHT = millis();
  }
  
  // Gioi han gia tri khong khi
  nhietDo = constrain(nhietDo, -40, 80);
  doAmKK = constrain(doAmKK, 0, 100);
  
  // Doc LDR (Anh sang - dao nguoc)
  int ldrRaw = analogRead(pinLDR);
  anhSang = map(ldrRaw, 0, 4095, 100, 0);
  anhSang = constrain(anhSang, 0, 100);
  
  // Doc cam bien dat TRUC TIEP tu chan ADC (khong MUX - on dinh hon cho Wokwi)
  // Phan chan: DoAmDat=34, pH=35, N=32, P=33, K=39(VN), EC=26
  
  // Do am dat (GPIO 34)
  int rawDoAmDat = analogRead(34);
  if (abs(rawDoAmDat - rawDoAmDatTruoc) > 200) {
    offsetDoAmDat = 0;
  }
  rawDoAmDatTruoc = rawDoAmDat;
  doAmDat = map(rawDoAmDat, 0, 4095, 0, 100) + (int)offsetDoAmDat;
  doAmDat = constrain(doAmDat, 0, 100);
  
  // pH dat (GPIO 35) - 0 den 14
  int rawPH = analogRead(35);
  phDat = rawPH / 4095.0f * 14.0f;
  phDat = constrain(phDat, 0.0f, 14.0f);
  
  // Nitrogen (GPIO 32)
  int rawN = analogRead(32);
  if (abs(rawN - rawNTruoc) > 100) offsetN = 0;
  rawNTruoc = rawN;
  n = map(rawN, 0, 4095, 0, 500) + (int)offsetN;
  n = constrain(n, 0, 500);
  
  // Phosphorus (GPIO 33)
  int rawP = analogRead(33);
  if (abs(rawP - rawPTruoc) > 100) offsetP = 0;
  rawPTruoc = rawP;
  p = map(rawP, 0, 4095, 0, 500) + (int)offsetP;
  p = constrain(p, 0, 500);
  
  // Potassium (GPIO 39 = VN)
  int rawK = analogRead(39);
  if (abs(rawK - rawKTruoc) > 100) offsetK = 0;
  rawKTruoc = rawK;
  k = map(rawK, 0, 4095, 0, 500) + (int)offsetK;
  k = constrain(k, 0, 500);
  
  // EC (GPIO 26)
  int rawEC = analogRead(26);
  if (abs(rawEC - rawECTruoc) > 100) offsetEC = 0;
  rawECTruoc = rawEC;
  ec = (rawEC / 4095.0f * 3.0f) + offsetEC;
  ec = constrain(ec, 0.0f, 3.0f);
}

// ==================== DIEU KHIEN THIET BI ====================

// Helper: Animation servo dao dong
void PlantZone::animateServo(bool dangChay, int& viTri, int& huong, uint8_t kenh) {
  if (dangChay) {
    viTri += huong * 15;
    if (viTri >= 180) { viTri = 180; huong = -1; }
    if (viTri <= 0) { viTri = 0; huong = 1; }
    pwm->setPWM(kenh, 0, map(viTri, 0, 180, 150, 600));
  } else {
    viTri = 90;
    pwm->setPWM(kenh, 0, 375);
  }
}

// Helper: Dat mau cho doan LED ring
void PlantZone::datMauVongLed(int pixelDau, int pixelCuoi, bool dangChay, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t mau = dangChay ? vongLed->Color(r, g, b) : 0;
  for (int i = pixelDau; i < pixelCuoi; i++) {
    vongLed->setPixelColor(i, mau);
  }
}

void PlantZone::dieuKhienQuat() {
  if (!cheTuDong) return;
  
  if (nhietDo > cauHinh->nhietDoMax) {
    quatChay = true;
  } else if (nhietDo < cauHinh->nhietDoMax - TRE_NHIET_DO) {
    quatChay = false;
  }
  
  animateServo(quatChay, viTriServoQuat, huongServoQuat, kenhQuat);
  
  if (quatChay) {
    offsetNhietDo -= TOC_DO_LAM_MAT;
  } else {
    if (offsetNhietDo < 0) offsetNhietDo += TOC_DO_PHUC_HOI_NHIET;
  }
}

void PlantZone::dieuKhienSuoi() {
  if (!cheTuDong) return;
  
  if (nhietDo < cauHinh->nhietDoMin) {
    suoiChay = true;
  } else if (nhietDo > cauHinh->nhietDoMin + TRE_NHIET_DO) {
    suoiChay = false;
  }
  
  datMauVongLed(0, 12, suoiChay, 255, 0, 0);
  
  if (suoiChay) {
    offsetNhietDo += TOC_DO_LAM_NONG;
  } else {
    if (offsetNhietDo > 0) offsetNhietDo -= TOC_DO_PHUC_HOI_NHIET;
  }
}

void PlantZone::dieuKhienPhunSuong() {
  if (!cheTuDong) return;
  
  if (doAmKK < cauHinh->doAmKKMin) {
    phunSuongChay = true;
  } else if (doAmKK > cauHinh->doAmKKMin + TRE_DO_AM_KK) {
    phunSuongChay = false;
  }
  
  datMauVongLed(12, 24, phunSuongChay, 0, 255, 255);
  
  if (phunSuongChay) {
    offsetDoAmKK += TOC_DO_TANG_DO_AM;
  } else {
    if (offsetDoAmKK > 0) offsetDoAmKK -= TOC_DO_GIAM_DO_AM;
  }
}

void PlantZone::dieuKhienDen() {
  if (!cheTuDong) return;
  
  if (anhSang < cauHinh->anhSangMin) {
    denChay = true;
  } else if (anhSang > cauHinh->anhSangMin + TRE_ANH_SANG) {
    denChay = false;
  }
  
  datMauVongLed(24, 36, denChay, 255, 0, 255);
}

void PlantZone::dieuKhienBomNuoc() {
  if (!cheTuDong) return;
  
  if (doAmDat < cauHinh->doAmDatMin) {
    bomNuocChay = true;
  } else if (doAmDat > cauHinh->doAmDatMin + TRE_DO_AM_DAT) {
    bomNuocChay = false;
  }
  
  animateServo(bomNuocChay, viTriServoBomNuoc, huongServoBomNuoc, kenhBomNuoc);
  
  if (bomNuocChay) {
    offsetDoAmDat += TOC_DO_TANG_DO_AM_DAT;
  } else {
    if (offsetDoAmDat > 0) offsetDoAmDat -= TOC_DO_GIAM_DO_AM_DAT;
  }
}

void PlantZone::dieuKhienBomN() {
  if (!cheTuDong) return;
  
  if (n < cauHinh->nMin) {
    bomNChay = true;
  } else if (n > cauHinh->nMin + TRE_DINH_DUONG) {
    bomNChay = false;
  }
  
  animateServo(bomNChay, viTriServoBomN, huongServoBomN, kenhBomN);
  
  if (bomNChay) {
    offsetN += TOC_DO_TANG_DINH_DUONG;
  } else {
    if (offsetN > 0) offsetN -= TOC_DO_GIAM_DINH_DUONG;
  }
}

void PlantZone::dieuKhienBomP() {
  if (!cheTuDong) return;
  
  if (p < cauHinh->pMin) {
    bomPChay = true;
  } else if (p > cauHinh->pMin + TRE_DINH_DUONG) {
    bomPChay = false;
  }
  
  animateServo(bomPChay, viTriServoBomP, huongServoBomP, kenhBomP);
  
  if (bomPChay) {
    offsetP += TOC_DO_TANG_DINH_DUONG;
  } else {
    if (offsetP > 0) offsetP -= TOC_DO_GIAM_DINH_DUONG;
  }
}

void PlantZone::dieuKhienBomK() {
  if (!cheTuDong) return;
  
  if (k < cauHinh->kMin) {
    bomKChay = true;
  } else if (k > cauHinh->kMin + TRE_DINH_DUONG) {
    bomKChay = false;
  }
  
  animateServo(bomKChay, viTriServoBomK, huongServoBomK, kenhBomK);
  
  if (bomKChay) {
    offsetK += TOC_DO_TANG_DINH_DUONG;
  } else {
    if (offsetK > 0) offsetK -= TOC_DO_GIAM_DINH_DUONG;
  }
}

void PlantZone::dieuKhienBomDinhDuong() {
  if (!cheTuDong) return;
  
  if (ec < cauHinh->ecMin) {
    bomDinhDuongChay = true;
  } else if (ec > cauHinh->ecMax - TRE_EC) {
    bomDinhDuongChay = false;
  }
  
  animateServo(bomDinhDuongChay, viTriServoBomDinhDuong, huongServoBomDinhDuong, kenhBomDinhDuong);
  
  if (bomDinhDuongChay) {
    offsetEC += TOC_DO_TANG_EC;
  } else {
    if (offsetEC > 0) offsetEC -= TOC_DO_GIAM_EC;
  }
}

void PlantZone::kiemTraCanhBao() {
  // Canh bao khi BAT KY chi so nao nam ngoai nguong an toan
  canhBao = false;
  
  // Kiem tra nhiet do (qua nong hoac qua lanh)
  if (nhietDo > cauHinh->nhietDoMax || nhietDo < cauHinh->nhietDoMin) {
    canhBao = true;
  }
  
  // Kiem tra do am khong khi (qua thap)
  if (doAmKK < cauHinh->doAmKKMin) {
    canhBao = true;
  }
  
  // Kiem tra anh sang (qua toi)
  if (anhSang < cauHinh->anhSangMin) {
    canhBao = true;
  }
  
  // Kiem tra do am dat (qua kho)
  if (doAmDat < cauHinh->doAmDatMin) {
    canhBao = true;
  }
  
  // Kiem tra pH dat (ngoai nguong)
  if (phDat < cauHinh->phMin || phDat > cauHinh->phMax) {
    canhBao = true;
  }
  
  // Kiem tra N, P, K (qua thap)
  if (n < cauHinh->nMin || p < cauHinh->pMin || k < cauHinh->kMin) {
    canhBao = true;
  }
  
  // Kiem tra EC (ngoai nguong)
  if (ec < cauHinh->ecMin || ec > cauHinh->ecMax) {
    canhBao = true;
  }
}


void PlantZone::dieuKhienThietBi() {
  kiemTraCanhBao();
  dieuKhienQuat();
  dieuKhienSuoi();
  dieuKhienPhunSuong();
  dieuKhienDen();
  dieuKhienBomNuoc();
  dieuKhienBomN();
  dieuKhienBomP();
  dieuKhienBomK();
  dieuKhienBomDinhDuong();
  capNhatChuong();
  
  // Cap nhat LED mot lan moi vong lap
  vongLed->show();
}

void PlantZone::capNhatChuong() {
  static bool trangThaiChuong = false;
  static unsigned long lanBatTatCuoi = 0;
  
  if (canhBao) {
    // Kieu bip: 200ms bat, 200ms tat
    if (millis() - lanBatTatCuoi > 200) {
      trangThaiChuong = !trangThaiChuong;
      if (trangThaiChuong) {
        tone(pinChuong, 2000);
      } else {
        noTone(pinChuong);
      }
      lanBatTatCuoi = millis();
    }
  } else {
    if (trangThaiChuong) {
      noTone(pinChuong);
      trangThaiChuong = false;
    }
  }
}

// ==================== DIEU KHIEN THU CONG ====================

void PlantZone::batTatQuat(bool bat) {
  if (cheTuDong) return;
  quatChay = bat;
  pwm->setPWM(kenhQuat, 0, bat ? 400 : 375);
}

void PlantZone::batTatSuoi(bool bat) {
  if (cheTuDong) return;
  suoiChay = bat;
  datMauVongLed(0, 12, bat, 255, 0, 0);
  vongLed->show();
}

void PlantZone::batTatPhunSuong(bool bat) {
  if (cheTuDong) return;
  phunSuongChay = bat;
  datMauVongLed(12, 24, bat, 0, 255, 255);
  vongLed->show();
}

void PlantZone::batTatDen(bool bat) {
  if (cheTuDong) return;
  denChay = bat;
  datMauVongLed(24, 36, bat, 255, 0, 255);
}

void PlantZone::batTatBomNuoc(bool bat) {
  if (cheTuDong) return;
  bomNuocChay = bat;
  pwm->setPWM(kenhBomNuoc, 0, bat ? 400 : 375);
}

void PlantZone::batTatBomDinhDuong(bool bat) {
  if (cheTuDong) return;
  bomDinhDuongChay = bat;
  pwm->setPWM(kenhBomDinhDuong, 0, bat ? 400 : 375);
}

// ==================== HIEN THI ====================

void PlantZone::capNhatManHinh(int yOffset) {
  if (!tft) return;
  
  char buffer[50];
  
  // Chi cap nhat TFT moi 1000ms de giam lag
  if (millis() - lanCapNhatTFT < 1000) return;
  lanCapNhatTFT = millis();
  
  // Header - Size 2
  tft->fillRect(0, yOffset, 240, 24, TFT_DARKGREY);
  tft->setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft->setTextFont(1);
  tft->setTextSize(2);
  sprintf(buffer, "VUNG %d: %s", maVung + 1, cauHinh->ten);
  tft->drawString(buffer, 5, yOffset + 4);
  
  int y = yOffset + 30;
  int chieuCaoDong = 18;
  
  // Nhiet do - Chi ve lai neu thay doi dang ke
  if (abs(nhietDo - hienThiNhietDoTruoc) > 0.1f) {
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    sprintf(buffer, "Nhiet do: %.1f do C   ", nhietDo);
    tft->drawString(buffer, 5, y);
    hienThiNhietDoTruoc = nhietDo;
  }
  
  // Do am khong khi
  y += chieuCaoDong;
  if (abs(doAmKK - hienThiDoAmKKTruoc) > 0.5f) {
    tft->setTextColor(TFT_BLUE, TFT_BLACK);
    sprintf(buffer, "Do am KK: %.0f %%      ", doAmKK);
    tft->drawString(buffer, 5, y);
    hienThiDoAmKKTruoc = doAmKK;
  }
  
  // Anh sang
  y += chieuCaoDong;
  if (abs(anhSang - hienThiAnhSangTruoc) > 1) {
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    sprintf(buffer, "Anh sang: %d %%       ", anhSang);
    tft->drawString(buffer, 5, y);
    hienThiAnhSangTruoc = anhSang;
  }
  
  // Do am dat
  y += chieuCaoDong;
  if (abs(doAmDat - hienThiDoAmDatTruoc) > 1) {
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    sprintf(buffer, "Do am dat: %d %%      ", doAmDat);
    tft->drawString(buffer, 5, y);
    hienThiDoAmDatTruoc = doAmDat;
  }
  
  // pH dat
  y += chieuCaoDong;
  if (abs(phDat - hienThiPHTruoc) > 0.1f) {
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
    sprintf(buffer, "pH dat: %.1f         ", phDat);
    tft->drawString(buffer, 5, y);
    hienThiPHTruoc = phDat;
  }
  
  // N
  y += chieuCaoDong;
  if (n != hienThiNTruoc) {
    tft->setTextColor(TFT_MAGENTA, TFT_BLACK);
    sprintf(buffer, "N: %d mg/kg          ", n);
    tft->drawString(buffer, 5, y);
    hienThiNTruoc = n;
  }

  // P
  y += chieuCaoDong;
  if (p != hienThiPTruoc) {
    tft->setTextColor(TFT_RED, TFT_BLACK);
    sprintf(buffer, "P: %d mg/kg          ", p);
    tft->drawString(buffer, 5, y);
    hienThiPTruoc = p;
  }

  // K
  y += chieuCaoDong;
  if (k != hienThiKTruoc) {
    tft->setTextColor(TFT_PINK, TFT_BLACK);
    sprintf(buffer, "K: %d mg/kg          ", k);
    tft->drawString(buffer, 5, y);
    hienThiKTruoc = k;
  }

  // EC
  y += chieuCaoDong;
  if (abs(ec - hienThiECTruoc) > 0.05f) {
    tft->setTextColor(TFT_ORANGE, TFT_BLACK);
    sprintf(buffer, "EC: %.2f mS/cm       ", ec);
    tft->drawString(buffer, 5, y);
    hienThiECTruoc = ec;
  }
  
  // Chi bao canh bao
  if (canhBao != hienThiCanhBaoTruoc) {
    if (canhBao) {
      tft->fillRect(210, yOffset, 30, 24, TFT_RED);
      tft->setTextColor(TFT_WHITE, TFT_RED);
      tft->drawString("!", 218, yOffset + 4);
    } else {
      tft->fillRect(210, yOffset, 30, 24, TFT_DARKGREY);
    }
    hienThiCanhBaoTruoc = canhBao;
  }
}

// ==================== CAP NHAT CHINH ====================

void PlantZone::update(int displayYOffset) {
  docCamBien();
  dieuKhienThietBi();
  capNhatManHinh(displayYOffset);
}
