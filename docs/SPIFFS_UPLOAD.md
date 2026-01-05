# Upload SPIFFS với Profiles

## Cách 1: Upload qua PlatformIO (Khuyến nghị)

### Bước 1: Build SPIFFS image

```bash
pio run --target buildfs
```

### Bước 2: Upload SPIFFS lên ESP32

```bash
pio run --target uploadfs
```

**Lưu ý**: Phải kết nối ESP32 qua USB trước khi upload.

---

## Cách 2: Upload thủ công cho Wokwi

Wokwi simulation có thể đọc SPIFFS nếu cấu hình đúng:

1. Trong `wokwi.toml`, thêm:

```toml
[[fs]]
mountpoint = "/spiffs"
files = "data"
```

2. File `profiles.json` sẽ được tự động load từ thư mục `data/`

---

## Cách 3: Fallback - Tạo profiles bằng Serial

Nếu SPIFFS không khả dụng, ProfileManager sẽ tự động:

- Tạo 3 profiles mặc định (Xà lách, Dâu tây, Cà chua)
- Lưu vào SPIFFS nếu có thể
- Sử dụng profiles trong RAM nếu SPIFFS failed

---

## Kiểm tra SPIFFS

Theo dõi Serial Monitor khi khởi động:

- `[OK] SPIFFS da khoi tao` - SPIFFS OK
- `[LOI] Khong the khoi tao SPIFFS!` - SPIFFS failed
- `Loaded: XA LACH (xa_lach)` - Profile loaded thành công

---

## Quản lý Profiles qua Serial/Webapp (Tương lai)

ProfileManager cung cấp các API:

- `loadProfiles()` - Load từ JSON
- `saveProfiles()` - Lưu vào JSON
- `themProfile(profile)` - Thêm profile mới
- `capNhatProfile(id, profile)` - Cập nhật profile
- `xoaProfile(id)` - Xóa profile
- `getProfileById(id)` - Lấy profile theo ID
