# 1. Perangkat dan Instalasi

## Arduino IDE dan board ESP32

1. Pasang **Arduino IDE 2.x** dari situs resmi Arduino.
2. Buka **File > Preferences**. Pada *Additional boards manager URLs*, tambahkan:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Buka **Boards Manager**, cari `esp32`, lalu pasang **esp32 by Espressif Systems versi 2.0.17**.

> **Penting:** gunakan versi **2.0.17**, bukan 3.x. Versi 3.x mengubah banyak API dan bisa membuat library Edge Impulse gagal di-compile. Jika Arduino IDE menawarkan update board atau library, pilih **LATER**.

4. Pilih board **AI Thinker ESP32-CAM** di menu **Tools > Board**, lalu pastikan **Tools > PSRAM: Enabled**.

## Menyambungkan ESP32-CAM

1. Pasang ESP32-CAM di atas shield **ESP32-CAM-MB**, lalu sambungkan ke PC dengan kabel USB data (bukan kabel khusus pengisian daya).
2. Pilih port di **Tools > Port** (misalnya `COM3` di Windows atau `/dev/ttyUSB0` di Linux).
3. Jika port tidak muncul, pasang driver **CH340**.

## Python untuk server

1. Pasang **Python 3.9 atau lebih baru**. Di Windows, centang *Add Python to PATH* saat instalasi.
2. Di folder `server`, jalankan:
   ```bash
   pip install -r requirements.txt
   ```

## Editor (opsional)

**VS Code** memudahkan menjalankan beberapa terminal sekaligus, misalnya satu untuk `server.py` dan satu untuk `simulasi.py`.

## Cek pemahaman

- [ ] Arduino IDE bisa mengenali board AI Thinker ESP32-CAM dan port-nya.
- [ ] Contoh sederhana (misalnya *File > Examples > 01.Basics > Blink*) bisa di-upload ke ESP32-CAM.
- [ ] `python --version` menampilkan versi 3.9 atau lebih baru.
