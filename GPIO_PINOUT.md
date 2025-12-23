# Bảng Phân Bổ GPIO - Smart Farm Dual Zone

## Zone 1 - Dâu Tây (Strawberry)

| Thiết Bị                 | Chân GPIO     | Ghi Chú           |
| ------------------------ | ------------- | ----------------- |
| **Cảm biến**             |               |                   |
| DHT22 (Nhiệt/Ẩm)         | GPIO 15       | Digital           |
| LDR (Ánh sáng)           | GPIO 27       | Analog (ADC2_CH7) |
| Potentiometer - Độ ẩm    | GPIO 36 (VP)  | Analog (ADC1_CH0) |
| Potentiometer - N (Đạm)  | GPIO 39 (VN)  | Analog (ADC1_CH3) |
| Potentiometer - P (Lân)  | GPIO 34       | Analog (ADC1_CH6) |
| Potentiometer - K (Kali) | GPIO 25       | Analog (ADC2_CH8) |
| Potentiometer - pH       | GPIO 32       | Analog (ADC1_CH4) |
| Potentiometer - EC       | GPIO 33       | Analog (ADC1_CH5) |
| **Thiết bị điều khiển**  |               |                   |
| LCD I2C                  | SDA/SCL       | Address 0x27      |
| Servo - Van nước         | GPIO 16 (RX2) | PWM               |
| Servo - Bơm N            | GPIO 17 (TX2) | PWM               |
| Servo - Bơm P            | GPIO 5        | PWM               |
| Servo - Bơm K            | GPIO 18       | PWM               |
| LED Ring - Grow Light    | GPIO 12       | NeoPixel          |
| LED Ring - Heater        | GPIO 14       | NeoPixel          |

## Zone 2 - Xà Lách (Lettuce)

| Thiết Bị                 | Chân GPIO    | Ghi Chú                     |
| ------------------------ | ------------ | --------------------------- |
| **Cảm biến**             |              |                             |
| DHT22 (Nhiệt/Ẩm)         | GPIO 4       | Digital                     |
| LDR (Ánh sáng)           | GPIO 26      | Analog (ADC2_CH9)           |
| Potentiometer - Độ ẩm    | GPIO 35      | Analog (ADC1_CH7)           |
| Potentiometer - N (Đạm)  | GPIO 39 (VN) | Analog (ADC1_CH3) ⚠️ Shared |
| Potentiometer - P (Lân)  | GPIO 36 (VP) | Analog (ADC1_CH0)           |
| Potentiometer - K (Kali) | GPIO 2       | Analog (ADC2_CH2)           |
| Potentiometer - pH       | GPIO 0       | Analog (ADC2_CH1)           |
| Potentiometer - EC       | GPIO 13      | Analog (ADC2_CH4)           |
| **Thiết bị điều khiển**  |              |                             |
| LCD I2C                  | SDA/SCL      | Address 0x26                |
| Servo - Van nước         | GPIO 19      | PWM                         |
| Servo - Bơm N            | GPIO 23      | PWM                         |
| Servo - Bơm P            | GPIO 21      | PWM ✅ Fixed                |
| Servo - Bơm K            | GPIO 22      | PWM ✅ Fixed                |
| LED Ring - Grow Light    | GPIO 16      | NeoPixel                    |
| LED Ring - Heater        | GPIO 17      | NeoPixel                    |

## Chân Dùng Chung (Shared)

| Chân        | Mục đích        | Lưu ý                                            |
| ----------- | --------------- | ------------------------------------------------ |
| GPIO 21, 22 | I2C (SDA, SCL)  | Cả 2 LCD dùng chung bus I2C                      |
| GPIO 39     | Potentiometer N | ⚠️ Cả 2 zone đọc cùng 1 chân - OK vì đọc tuần tự |

## Thay Đổi Đã Sửa

### Vấn đề trước:

- Zone 2 Servo P: GPIO 3 (Boot pin - conflict)
- Zone 2 Servo K: GPIO 1 (TX0 - Serial conflict)

### Đã sửa thành:

- Zone 2 Servo P: GPIO 21 ✅
- Zone 2 Servo K: GPIO 22 ✅

## Lưu Ý Quan Trọng

⚠️ **GPIO 39 (VN)** được dùng chung cho Potentiometer N của cả 2 zone. Điều này OK vì:

- Cả 2 zone đọc tuần tự trong `loop()`
- Không có xung đột vật lý
- Nếu muốn hoàn toàn độc lập, có thể thay GPIO 39 của Zone 2 bằng chân khác

✅ **Tất cả các chân khác đã hoàn toàn độc lập giữa 2 zone**

## Cách Kiểm Tra

1. **Chạy Wokwi Simulator**
2. **Kiểm tra LCD**:
   - LCD trái (0x27) hiển thị "Z1" và "STRAWBERRY"
   - LCD phải (0x26) hiển thị "Z2" và "LETTUCE"
3. **Test độc lập**:
   - Vặn potentiometer Zone 1 → Chỉ servo Zone 1 chuyển động
   - Vặn potentiometer Zone 2 → Chỉ servo Zone 2 chuyển động
4. **Test LED Ring**:
   - Thay đổi nhiệt độ DHT22 Zone 1 → Chỉ LED Heat Zone 1 phản ứng
   - Thay đổi ánh sáng LDR Zone 2 → Chỉ LED Grow Zone 2 phản ứng
