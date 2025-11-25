# 🔔 TINGBOX – IoT Notify & Payment Display (ESP32)

**TINGBOX** là thiết bị IoT mini sử dụng **ESP32**, có khả năng lấy thông báo real-time và hiển thị giao dịch nhận tiền từ **Seapay API**.
Thiết bị đi kèm **màn hình OLED 1.1 inch**, loa thông báo thông qua **mạch khuếch đại âm thanh**, và cơ chế **Cloudflare bypass** giúp truy cập API an toàn và ổn định.

---

##  Chức năng chính

### 1. Nhận & hiển thị thông báo

* ESP32 tự kết nối server thông qua API.
* Nhận thông báo dạng:

  * Tin nhắn đẩy
  * Cảnh báo sự kiện
  * Thông báo hệ thống
* Hiển thị trực tiếp lên màn OLED 1.1".

### 2. Hiển thị giao dịch nhận tiền (Seapay)

* Gọi API Seapay liên tục hoặc theo webhook/polling.
* Hiển thị:

  * Số tiền + đơn vị
  * Nội dung giao dịch
  * Thời gian
  * Tên ví/tài khoản
* Phát âm thanh báo giao dịch qua module ampli.

---

## Phần cứng sử dụng

| Linh kiện                                    | Mô tả                          |
| -------------------------------------------- | ------------------------------ |
| **ESP32 Devkit V1**                          | MCU chính                      |
| **OLED 1.1" (SSD1306/SH1106)**               | Màn hình hiển thị              |
| **Mạch khuếch đại âm thanh MAX98357 **       | Phát âm thanh báo              |
| **Loa mini**                                 | Chuông báo nhận tiền           |
| **Nguồn 5V – 2A**                            | Cấp nguồn cho toàn bộ thiết bị |

---

## Kiến trúc hoạt động

```
+--------------+        +------------------------+
|  TINGBOX     | <----> |  Cloudflare Bypass Proxy |
| ESP32        |        |  (Server trung gian)   |
+--------------+        +------------------------+
                                 |
                                 | Proxy Skip / Token Generator
                                 v
+----------------------------------------+
|              Seapay API               |
+----------------------------------------+
```

### Vì sao cần Cloudflare bypass?

* ESP32 không xử lý được JS challenge của Cloudflare.
* Bạn dùng **server backend** làm “trung gian” để:

  * Chạy request hợp lệ
  * Tạo cookie/session/token
  * Trả dữ liệu sạch về cho ESP32


## Cài đặt & Upload Firmware

### 1. Clone dự án

```bash
git clone https://github.com/AzenKain/IOT_TINGBOX.git
cd IOT_TINGBOX
```

## Giao diện hiển thị trên OLED

Ví dụ:

```
You received:
+ 150,000 VND
```

---

## Âm thanh báo giao dịch

* Khi có giao dịch mới: phát chuông “pling”
* Khi có thông báo: ting ngắn
* Loa → ampli → ESP32 PWM

---

## License

MIT License — tự do sử dụng.

