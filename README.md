# Smart Farm - Single Zone (Dâu Tây)

## Tổng Quan

Hệ thống Smart Farm đơn giản hóa với **1 zone** trồng **Dâu Tây (Strawberry)**, sử dụng 1 ESP32 để điều khiển tất cả cảm biến và thiết bị.

## Cấu Trúc Hệ Thống

### Phần Cứng

- **1 ESP32 DevKit V1**
- **1 LCD 2004 (I2C 0x27)** - Hiển thị thông tin
- **Cảm biến môi trường**:
  - DHT22 (Nhiệt độ & Độ ẩm không khí)
  - LDR (Cảm biến ánh sáng)
- **Cảm biến đất** (6 potentiometers giả lập):
  - Độ ẩm đất
  - N (Đạm)
  - P (Lân)
  - K (Kali)
  - pH
  - EC (Độ dẫn điện)
- **Thiết bị điều khiển**:
  - 4 Servo (Van nước, Bơm N/P/K)
  - 2 LED Ring (Đèn Grow, Máy sưởi)

### Phần Mềm

- **SmartZone Class**: Đóng gói toàn bộ logic điều khiển
- **Crop-specific thresholds**: Ngưỡng tối ưu cho Dâu Tây

## Phân Bổ GPIO

| Thiết Bị                | GPIO              | Loại     |
| ----------------------- | ----------------- | -------- |
| **Cảm biến**            |                   |          |
| DHT22                   | 15                | Digital  |
| LDR                     | 27                | Analog   |
| Độ ẩm đất               | 36 (VP)           | Analog   |
| N (Đạm)                 | 39 (VN)           | Analog   |
| P (Lân)                 | 34                | Analog   |
| K (Kali)                | 25                | Analog   |
| pH                      | 32                | Analog   |
| EC                      | 33                | Analog   |
| **Thiết bị điều khiển** |                   |          |
| LCD I2C                 | 21, 22 (SDA, SCL) | I2C      |
| Van nước                | 16 (RX2)          | PWM      |
| Bơm N                   | 17 (TX2)          | PWM      |
| Bơm P                   | 5                 | PWM      |
| Bơm K                   | 18                | PWM      |
| Đèn Grow                | 12                | NeoPixel |
| Máy sưởi                | 14                | NeoPixel |

## Ngưỡng Điều Khiển (Dâu Tây)

| Thông Số  | Ngưỡng Tối Thiểu | Hành Động    |
| --------- | ---------------- | ------------ |
| Độ ẩm đất | 35%              | Mở van nước  |
| N (Đạm)   | 250 mg/kg        | Bật bơm N    |
| P (Lân)   | 300 mg/kg        | Bật bơm P    |
| K (Kali)  | 350 mg/kg        | Bật bơm K    |
| Nhiệt độ  | < 20°C           | Bật máy sưởi |
| Ánh sáng  | < 30%            | Bật đèn Grow |

## Hiển Thị LCD

```
Z1 T:25C H:65%
M:42% pH:6 L:45%
N:320 P:280 K:400
W:OK N:OK P:ON
```

**Giải thích:**

- Dòng 1: Zone ID, Nhiệt độ, Độ ẩm không khí
- Dòng 2: Độ ẩm đất (M), pH, Ánh sáng (L)
- Dòng 3: Giá trị NPK
- Dòng 4: Trạng thái thiết bị (W=Water, N/P/K=Pumps)

## Cấu Trúc Code

### [main.cpp](file:///e:/TLU/SmartFarm/src/main.cpp) - 27 dòng

```cpp
SmartZone* farm = nullptr;

void setup() {
  farm = new SmartZone(1);
  farm->setCrop(CROP_STRAWBERRY);
  farm->begin();
}

void loop() {
  farm->run();
  delay(200);
}
```

### [SmartZone.h](file:///e:/TLU/SmartFarm/include/SmartZone.h)

- Định nghĩa class SmartZone
- Enum CropType (STRAWBERRY, LETTUCE)
- Cấu trúc Pins và Thresholds

### [SmartZone.cpp](file:///e:/TLU/SmartFarm/src/SmartZone.cpp)

- Constructor: Khởi tạo pins và hardware
- `setCrop()`: Thiết lập loại cây
- `begin()`: Khởi tạo thiết bị
- `run()`: Vòng lặp chính (đọc cảm biến + điều khiển)

## Cách Sử Dụng

### 1. Build Project

```bash
platformio run
```

### 2. Chạy Wokwi Simulator

- Mở VS Code
- F1 → "Wokwi: Start Simulator"

### 3. Tương Tác

- **Vặn potentiometer** để thay đổi giá trị cảm biến
- **Quan sát LCD** để xem thông tin real-time
- **Xem servo và LED** phản ứng theo ngưỡng

### 4. Thay Đổi Loại Cây

Trong `main.cpp`, thay đổi:

```cpp
farm->setCrop(CROP_LETTUCE);  // Chuyển sang Xà lách
```

Ngưỡng sẽ tự động điều chỉnh:

- Độ ẩm: 35% → 50%
- N: 250 → 400 mg/kg
- K: 350 → 250 mg/kg

## Files Quan Trọng

| File             | Dòng | Mô Tả                |
| ---------------- | ---- | -------------------- |
| `diagram.json`   | 233  | Sơ đồ Wokwi (1 zone) |
| `main.cpp`       | 27   | Entry point          |
| `SmartZone.h`    | 62   | Class header         |
| `SmartZone.cpp`  | 216  | Class implementation |
| `platformio.ini` | 15   | Build config         |

## Tính Năng

✅ **Tự động tưới** khi độ ẩm thấp  
✅ **Tự động bón phân** NPK theo ngưỡng  
✅ **Điều khiển ánh sáng** (đèn Grow)  
✅ **Điều khiển nhiệt độ** (máy sưởi)  
✅ **Hiển thị real-time** trên LCD  
✅ **Ngưỡng tùy chỉnh** theo loại cây

## Mở Rộng Trong Tương Lai

- [ ] Thêm cảm biến thực (thay potentiometer)
- [ ] Kết nối WiFi/Bluetooth
- [ ] Lưu dữ liệu lịch sử
- [ ] Dashboard web
- [ ] Cảnh báo qua email/SMS
- [ ] Điều khiển từ xa
