# 5. Troubleshooting

## Compile

**`fatal error: opening dependency file ... No such file or directory`** (Windows)
Path terlalu panjang. Nama library Edge Impulse panjang dan folder SDK-nya bertingkat.
1. Tutup Arduino IDE.
2. Di `Documents\Arduino\libraries\`, ganti nama folder library menjadi pendek, misalnya `ei_person`. Di dalamnya, ubah baris pertama `library.properties` menjadi `name=ei_person`. Baris `#include` di sketch tidak perlu diubah.
3. Hapus folder `C:\Users\<nama>\AppData\Local\arduino\sketches`.
4. (Opsional, perlu hak admin) Buka PowerShell **sebagai Administrator** lalu jalankan perintah berikut, kemudian restart PC:
   ```powershell
   New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
   ```

**Error setelah update board ESP32**
Kembalikan **esp32 by Espressif Systems** ke versi **2.0.17** di Boards Manager.

**`Model harus berjenis Object Detection (FOMO)`**
Library yang dipasang bukan model Object Detection. Ekspor ulang dari proyek FOMO.

## Upload

**`Could not connect to COM3 serial port. Already connected`**
Serial Monitor masih terbuka. Tutup tab Serial Monitor, lalu upload ulang.

**`Failed to connect to ESP32: Timed out waiting for packet header`**
Coba kabel USB lain, tekan dan tahan tombol IO0 saat upload dimulai, atau pastikan shield ESP32-CAM-MB terpasang dengan benar.

## Saat berjalan

**`Kamera gagal dinyalakan`**
Periksa konektor pita kamera dan daya. Gunakan adaptor 5V 2A.

**ESP32 restart sendiri (`Brownout detector was triggered`)**
Daya kurang. Ganti adaptor atau kabel USB.

**Karakter acak di awal Serial Monitor**
Normal. Itu pesan bawaan ESP32 saat menyala dengan baud rate berbeda.

## WiFi

Firmware menampilkan kode status jika gagal tersambung (mode STA):

| Kode | Arti | Perbaikan |
|---|---|---|
| 1 | Nama WiFi tidak ditemukan | Pastikan nama benar, hotspot memakai 2,4 GHz, dan jaraknya dekat |
| 4 | Gagal terhubung | Periksa password; ubah keamanan hotspot dari WPA3 ke WPA2 |
| 6 | Belum terhubung | Tunggu, atau matikan lalu nyalakan hotspot |

Firmware juga memindai dan menampilkan daftar WiFi yang terlihat oleh ESP32, untuk memastikan hotspot terdeteksi.

**Hotspot kampus tidak bisa dipakai**
Jaringan dengan halaman login (*captive portal*) atau isolasi antarperangkat tidak bisa dipakai. Gunakan mode AP atau hotspot HP.

**`Gagal kirim ke server` (mode STA)**
- Pastikan `SERVER_URL` memakai IP PC yang benar, dan PC berada di jaringan yang sama.
- Di Windows, ubah profil jaringan menjadi **Private** (*Settings > Network & internet > Wi-Fi > nama jaringan*), atau izinkan Python di Windows Firewall.

**`Kamera http://192.168.4.1/data belum bisa dihubungi` (mode AP)**
PC belum tersambung ke WiFi `Penghitung-Pengunjung`. Pesan ini hilang otomatis setelah PC tersambung.

**WiFi `Penghitung-Pengunjung` tidak muncul**
Pastikan Serial Monitor menampilkan `WiFi "Penghitung-Pengunjung" aktif`. Jika masih tertulis `Menyambung WiFi...`, firmware yang berjalan belum versi mode AP. Segarkan daftar WiFi di PC dan dekatkan PC ke ESP32.

## Simulasi

**`urlopen error timed out` saat menjalankan `simulasi.py`**
Pastikan `server.py` sedang berjalan di terminal lain. Gunakan alamat `127.0.0.1`, bukan `localhost`.
