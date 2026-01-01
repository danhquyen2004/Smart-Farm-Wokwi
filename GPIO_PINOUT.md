# Bảng Phân Bổ GPIO - Smart Farm v2.1 (Dual MUX Edition)

## 🎯 Thiết kế mới: Dual CD74HC4067 Multiplexer

Thiết kế này sử dụng **2 CD74HC4067** chia sẻ cùng đường điều khiển S0-S3, cho phép:

- ✅ **Không xung đột GPIO** - Keypad đọc qua MUX2
- ✅ **Tiết kiệm GPIO** - Chỉ cần 1 GPIO thêm (D35) cho MUX2
- ✅ **Dễ mở rộng** - Còn nhiều kênh trống trên cả 2 MUX

---

## Tổng quan hệ thống

| Thành phần    | Số lượng | Giao tiếp              |
| ------------- | -------- | ---------------------- |
| TFT ILI9341   | 3        | SPI (shared)           |
| Keypad 4x4    | 1        | **Via MUX2**           |
| CD74HC4067    | **2**    | Digital + ADC          |
| PCA9685       | 1        | I2C                    |
| DHT22         | 2        | Digital                |
| LDR           | 2        | Analog                 |
| Buzzer        | 2        | Digital                |
| LED Ring      | 6        | NeoPixel (daisy chain) |
| Servo         | 12       | PWM via PCA9685        |
| Potentiometer | 12       | Analog via MUX1        |

---

## Sơ đồ kết nối Dual MUX

```
                    ┌───────────────────────────────────────────┐
                    │              ESP32-DEVKIT-V1              │
                    └───────────────────────────────────────────┘
                         │     │     │     │     │     │
                        D16   D17    D5   RX2   D34   D35
                         │     │     │     │     │     │
              ┌──────────┴─────┴─────┴─────┴─────┘     │
              │                                         │
        ┌─────┴─────┐                           ┌──────┴─────┐
        │   MUX1    │                           │    MUX2    │
        │ CD74HC4067│                           │ CD74HC4067 │
        │ (Sensors) │                           │  (Keypad)  │
        └───────────┘                           └────────────┘
             │                                        │
        S0 S1 S2 S3                              S0 S1 S2 S3
        (SHARED!)                                (SHARED!)
             │                                        │
      COM → D34 (ADC)                          COM → D35 (Digital)
             │                                        │
        I0-I11: Pots                            I0-I3: Columns
        I12-I15: Free                           I4-I7: Rows
                                                I8-I15: Free
```

---

## Chi tiết GPIO ESP32-DEVKIT-V1

### 🔌 Multiplexer Control (SHARED)

| Tín hiệu | GPIO    | Ghi chú               |
| -------- | ------- | --------------------- |
| S0       | GPIO 16 | Shared by MUX1 & MUX2 |
| S1       | GPIO 17 | Shared by MUX1 & MUX2 |
| S2       | GPIO 5  | Shared by MUX1 & MUX2 |
| S3       | GPIO 4  | Shared with TFT RST   |

### 📊 MUX1 - Analog Sensors

| Tín hiệu | GPIO/Channel   | Chức năng                      |
| -------- | -------------- | ------------------------------ |
| COM      | GPIO 34 (ADC1) | Analog input                   |
| I0-I5    | Channels       | Zone 1: Moist, pH, N, P, K, EC |
| I6-I11   | Channels       | Zone 2: Moist, pH, N, P, K, EC |
| I12-I15  | Channels       | **Reserved for expansion**     |

### ⌨️ MUX2 - Keypad

| Tín hiệu | GPIO/Channel | Chức năng                  |
| -------- | ------------ | -------------------------- |
| COM      | GPIO 35      | Digital input with pullup  |
| I0       | Channel      | Keypad Column 1            |
| I1       | Channel      | Keypad Column 2            |
| I2       | Channel      | Keypad Column 3            |
| I3       | Channel      | Keypad Column 4            |
| I4       | Channel      | Keypad Row 1               |
| I5       | Channel      | Keypad Row 2               |
| I6       | Channel      | Keypad Row 3               |
| I7       | Channel      | Keypad Row 4               |
| I8-I15   | Channels     | **Reserved for expansion** |

---

### 🖥️ TFT Displays (SPI Bus)

| TFT     | CS Pin  | Chức năng                            |
| ------- | ------- | ------------------------------------ |
| tftMain | GPIO 14 | Main Control Panel (Login/Dashboard) |
| tft1    | GPIO 15 | Zone 1 Display                       |
| tft2    | GPIO 27 | Zone 2 Display                       |

**Shared SPI Pins:**
| Tín hiệu | GPIO |
|----------|------|
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK | GPIO 18 |
| RST | GPIO 4 |
| D/C | GPIO 2 |

---

### 🌡️ Cảm biến

| Thiết bị | Zone   | GPIO         | Ghi chú  |
| -------- | ------ | ------------ | -------- |
| DHT22 #1 | Zone 1 | GPIO 25      | Digital  |
| DHT22 #2 | Zone 2 | GPIO 26      | Digital  |
| LDR #1   | Zone 1 | GPIO 36 (VP) | ADC1_CH0 |
| LDR #2   | Zone 2 | GPIO 39 (VN) | ADC1_CH3 |

---

### 🔊 Buzzer (Alarm)

| Buzzer  | Zone   | GPIO    |
| ------- | ------ | ------- |
| buzzer1 | Zone 1 | GPIO 33 |
| buzzer2 | Zone 2 | GPIO 32 |

---

### 💡 LED Rings (NeoPixel)

| LED Ring  | Zone   | GPIO        | Pixel Count |
| --------- | ------ | ----------- | ----------- |
| ring1Heat | Zone 1 | GPIO 12     | 12          |
| ring1Mist | Zone 1 | Daisy chain | 12          |
| ring1Grow | Zone 1 | Daisy chain | 12          |
| ring2Heat | Zone 2 | GPIO 13     | 12          |
| ring2Mist | Zone 2 | Daisy chain | 12          |
| ring2Grow | Zone 2 | Daisy chain | 12          |

---

### 🔧 PCA9685 (I2C PWM Driver)

| Tín hiệu | GPIO    |
| -------- | ------- |
| SDA      | GPIO 21 |
| SCL      | GPIO 22 |

**Servo Channels:**
| Channel | Thiết bị | Zone |
|---------|----------|------|
| LED0-5 | Fan, Water, N, P, K, Nutrient | Zone 1 |
| LED6-11 | Fan, Water, N, P, K, Nutrient | Zone 2 |
| LED12-15 | **Reserved** | - |

---

### 🔘 Button

| Button  | GPIO   | Chức năng      |
| ------- | ------ | -------------- |
| btnMode | GPIO 0 | Mode selection |

---

## Keypad Layout

```
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  A  │  ← A = Zone 1 (quick select)
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │  B  │  ← B = Zone 2 (quick select)
├─────┼─────┼─────┼─────┤
│  7  │  8  │  9  │  C  │  ← C = Cancel/Back
├─────┼─────┼─────┼─────┤
│  *  │  0  │  #  │  D  │  ← D = Confirm, * = Settings, # = Lock
└─────┴─────┴─────┴─────┘
```

---

## Tổng hợp GPIO đã sử dụng

| GPIO    | Chức năng        | Loại          |
| ------- | ---------------- | ------------- |
| 0       | Button Mode      | Input         |
| 2       | TFT D/C          | Output        |
| 4       | TFT RST          | Output        |
| 5       | MUX S2           | Output        |
| 12      | LED Ring Z1      | Output        |
| 13      | LED Ring Z2      | Output        |
| 14      | TFT Main CS      | Output        |
| 15      | TFT1 CS          | Output        |
| 16      | MUX S0 + RX2(S3) | Output        |
| 17      | MUX S1           | Output        |
| 18      | SPI SCK          | Output        |
| 19      | SPI MISO         | Input         |
| 21      | I2C SDA          | I/O           |
| 22      | I2C SCL          | Output        |
| 23      | SPI MOSI         | Output        |
| 25      | DHT1             | I/O           |
| 26      | DHT2             | I/O           |
| 27      | TFT2 CS          | Output        |
| 32      | Buzzer2          | Output        |
| 33      | Buzzer1          | Output        |
| 34      | MUX1 COM         | ADC Input     |
| 35      | MUX2 COM         | Digital Input |
| 36 (VP) | LDR1             | ADC Input     |
| 39 (VN) | LDR2             | ADC Input     |

**Tổng GPIO sử dụng: 24/34**
**GPIO còn trống: 1, 3 (TX/RX0)**

---

## Hướng dẫn sử dụng

### Đăng nhập

1. Nhập mật khẩu 4 số (mặc định: `1234`)
2. Nhấn `D` để xác nhận
3. Nhấn `C` để xóa

### Dashboard

- Nhấn `A` để vào Zone 1 Control
- Nhấn `B` để vào Zone 2 Control
- Nhấn `*` để vào Settings
- Nhấn `#` để khóa hệ thống

### Zone Control (Manual Mode)

- Nhấn `1` để bật/tắt Fan
- Nhấn `2` để bật/tắt Heater
- Nhấn `3` để bật/tắt Mist
- Nhấn `4` để bật/tắt Grow LED
- Nhấn `5` để bật/tắt Water pump
- Nhấn `6` để bật/tắt Nutrient pump
- Nhấn `D` để chuyển Auto/Manual
- Nhấn `C` để quay lại Dashboard

---

## Lợi ích của thiết kế Dual MUX

1. ✅ **Không xung đột GPIO** - Tất cả keypad pins đều đi qua MUX2
2. ✅ **Tiết kiệm GPIO** - Chỉ cần thêm 1 GPIO (D35) thay vì 8
3. ✅ **Chia sẻ S0-S3** - Cả 2 MUX dùng chung đường điều khiển
4. ✅ **Dễ mở rộng** - Còn 4 kênh trống trên MUX1, 8 kênh trên MUX2
5. ✅ **Serial debug hoạt động** - Không dùng TX0/RX0
