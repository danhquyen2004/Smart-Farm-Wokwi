# TÀI LIỆU THIẾT KẾ & KỊCH BẢN HỆ THỐNG SMART FARM (MÔ PHỎNG)

## 1. TỔNG QUAN HỆ THỐNG

Dự án xây dựng mô hình mô phỏng hệ thống nông nghiệp thông minh đa vùng (Multi-zone Smart Farm)
trên nền tảng ESP32. Hệ thống có khả năng giám sát và điều khiển tự động môi trường cho 3 khu vực
canh tác riêng biệt, đồng thời hỗ trợ các chế độ chăm sóc khác nhau tùy thuộc vào loại cây trồng
(Profile-based Control).
Vi điều khiển trung tâm: ESP32 DevKit V1.
Giao diện hiển tḥ: Hệ thống 3 màn hình LCD 2004 (I2C) độc lập cho từng khu vực.
Cơ chế cảnh báo: Còi Buzzer và đèn báo trạng thái.
Chế độ vận hành: Tự động hoàn toàn (Automation) dựa trên ngưỡng cài đặt của từng loại cây.

## 2. PHÂN KHU CHỨC NĂNG (ZONING)

Hệ thống được chia thành 3 phân khu (Zone) với các nhiệm vụ chuyên biệt:

ZONE A: MÔI TRƯỜNG KHÔNG KHÍ (AIR CONTROL)
_Mục tiêu: Duy trì vi khí hậu lý tưởng cho nhà màng/nhà kính._

```
Chức năng Thiết ḅ Đo
(Input)
```

```
Thiết ḅ Chấp hành
(Output)
```

```
Cơ chế Mô phỏng (Visual)
```

```
Giám sát Nhiệt
độ
```

```
Cảm biến DHT22 Quạt làm mát (Ventilation
Fan)
```

```
Servo (Cánh chữ thập): Quay 180°
khi bật.
Máy sưởi (Heater) Vòng LED (Đỏ): Sáng rực khi sưởi ấm.
Giám sát Độ ẩm Cảm biến DHT22 Máy phun sương
(Humidifier)
```

```
Vòng LED (Xanh lơ): Sáng khi phun
sương.
Giám sát Ánh
sáng
```

```
Quang trở (LDR) Đèn quang hợp (Grow
Light)
```

```
Vòng LED (Tím): Sáng khi trời tối.
```

### ZONE B: MÔI TRƯỜNG ĐẤT (SOIL / PRECISION AG)

_Mục tiêu: Nông nghiệp chính xác, châm phân tự động theo nồng độ N-P-K._

```
Chức năng Thiết ḅ Đo (Input) Thiết ḅ Chấp hành (Output) Cơ chế Mô phỏng (Visual)
Tưới tiêu Biến trở: Độ ẩm đất Van nước chính Servo (Xanh lá): Gạt mở van 90°.
```

```
Châm phân N Biến trở: Nồng độ N Bơm đ̣nh lượng Đạm Servo (Đỏ): Gạt mở van.
Châm phân P Biến trở: Nồng độ P Bơm đ̣nh lượng Lân Servo (Vàng): Gạt mở van.
Châm phân K Biến trở: Nồng độ K Bơm đ̣nh lượng Kali Servo (Cam): Gạt mở van.
An toàn Đất Biến trở: pH & EC Còi báo động (Buzzer) Kêu khi EC quá cao hoặc pH lỗi.
```

### ZONE C: MÔI TRƯỜNG THỦY CANH (HYDROPONICS)

_Mục tiêu: Kiểm soát nhiệt độ và dinh dưỡng dung ḍch thủy canh._

```
Chức năng Thiết ḅ Đo (Input) Thiết ḅ Chấp hành
(Output)
```

```
Cơ chế Mô phỏng (Visual)
```

```
Nhiệt độ
nước
```

```
Cảm biến DS18B20 Máy làm lạnh (Chiller) Servo (Xanh dương): Quay khi nước
nóng.
Dinh dưỡng Biến trở: EC (Độ dẫn
điện)
```

```
Bơm dung ḍch A/B Servo (Trắng): Gạt mở van châm
phân.
Độ pH Nước Biến trở: pH Màn hình cảnh báo Hiển tḥ trạng thái pH.
```

## 3. CƠ CHẾ "PROFILE CÂY TRỒNG" (PLANT PROFILES)

Hệ thống không chạy theo một thông số cố đ̣nh mà thay đổi ngưỡng kích hoạt (Thresholds) dựa trên
loại cây người dùng chọn qua nút bấm MODE.

Profile 1: Cây Xà Lách (Lettuce)
Đặc tính: Ưa mát, cần độ ẩm cao, ánh sáng trung bình, dinh dưỡng thấp.
Ngưỡng nhiệt độ: > 28 °C là nóng (Bật quạt).
Ngưỡng độ ẩm đất: < 80 % là khô (Tưới nước).
Dinh dưỡng: Cần nhiều Đạm (N), ít Kali (K). Độ mặn (EC) thấp < 1.2.

Profile 2: Cây Dưa Lưới (Melon)
Đặc tính: Cḥu nhiệt tốt, ưa khô ráo, cần nhiều nắng và Kali để tạo ngọt.
Ngưỡng nhiệt độ: > 35 °C mới bật quạt.
Ngưỡng độ ẩm đất: < 50 % mới tưới (Tưới ít).
Dinh dưỡng: Cần rất nhiều Kali (K). Cḥu được độ mặn cao (EC < 2.5).

Profile 3: Cây Dâu Tây (Strawberry)

```
Đặc tính: Cây ôn đới, ưa lạnh, nhạy cảm với môi trường.
Ngưỡng nhiệt độ: > 24 °C là nóng (Bật quạt sớm).
Dinh dưỡng: Cân bằng N-P-K.
```

## 4. KỊCH BẢN VẬN HÀNH CHI TIẾT (OPERATION FLOW)

Ḳch bản 1: Điều hòa Vi khí hậu (Zone A)

1. Phát hiện: Cảm biến DHT22 đọc nhiệt độ môi trường.
2. So sánh: Nếu Nhiệt độ > Max_Temp của cây đang chọn.
3. Hành động:
   Servo "QUẠT KHÍ" quay (mô phỏng quạt thông gió).
   Nếu nhiệt độ quá cao, Vòng LED Xanh (Phun sương) bật để hỗ trợ hạ nhiệt.
   Màn hình LCD 1 hiển tḥ: FAN: ON [COOLING].
4. Kết thúc: Khi nhiệt độ giảm xuống dưới ngưỡng, Quạt và Phun sương tự tắt.

Ḳch bản 2: Bón phân thông minh & An toàn (Zone B)

1. Bước 1 - Kiểm tra An toàn (Priority High):
   Hệ thống đọc chỉ số EC (Độ mặn) và pH đất trước tiên.
   Sự cố: Nếu EC quá cao (Đất ḅ nhiễm mặn/dư phân).
   Xử lý:
   Còi Buzzer kêu báo động.
   Hệ thống KHÓA CHẶT tất cả bơm phân bón (Ngắt Servo N, P, K).
   Mở Van Nước để rửa trôi muối (Leaching).
   Màn hình LCD 2 báo: ALARM: EC HIGH!.
2. Bước 2 - Chăm sóc (Nếu an toàn):
   Nếu đất khô -> Mở Servo Nước.
   Nếu thiếu Đạm/Lân/Kali -> Mở Servo bơm tương ứng.
   Màn hình hiển tḥ: ADDING: N P K.

Ḳch bản 3: Quản lý Thủy canh (Zone C)

1. Giám sát nhiệt: Cảm biến DS18B20 đo nhiệt độ nước trong ống.
2. Xử lý: Nếu nước nóng quá mức cḥu đựng của rễ (ví dụ > 30 °C với Xà lách) -> Bật Servo Chiller
   (Máy làm lạnh nước).
3. Ổn đ̣nh dinh dưỡng: Nếu cảm biến EC báo nước ḅ loãng (cây ăn hết phân) -> Bật Servo Bơm DD
   để châm thêm dung ḍch A/B đậm đặc.

## ƯỚ Ẫ Ể Ử Ê

## 5. HƯỚNG DẪN KIỂM THỬ TRÊN WOKWI

Để demo đồ án thành công, cần thực hiện các thao tác sau:

1. Khởi động: Bấm Play. Quan sát quá trình "Startup Sequence" (Đèn nháy, Servo quay) để chứng
   minh phần cứng hoạt động tốt.
2. Chuyển chế độ: Nhấn nút MODE. Quan sát màn hình LCD thay đổi tên cây và các ngưỡng cài
   đặt.
3. Tạo tình huống giả đ̣nh:
   Kéo thanh nhiệt độ lên cao -> Xem Quạt quay.
   Kéo thanh ánh sáng xuống thấp -> Xem Đèn Grow (Màu tím) sáng.
   Vặn biến trở EC Đất lên cao -> Xem Còi báo động và quy trình Khóa hệ thống.

```
### Lời khuyên cuối cùng:
Bạn có thể copy nội dung trên, lưu thành file word và nộp kèm với code. Đây là một bản m
Chúc bạn đạt điểm tối đa với đồ án này nhé!
```
