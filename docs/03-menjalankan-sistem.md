# 3. Menjalankan Sistem

## Pilih mode jaringan

Atur di bagian **KONFIGURASI** pada `esp32_penghitung.ino`.

| | Mode AP (bawaan) | Mode STA |
|---|---|---|
| Pengaturan | `MODE_AP = true` | `MODE_AP = false` |
| Cara kerja | ESP32 membuat WiFi sendiri, PC menyambung ke ESP32 | ESP32 dan PC menyambung ke hotspot/router yang sama |
| Arah data | `server.py` mengambil data dari `http://192.168.4.1/data` | ESP32 mengirim data ke `SERVER_URL` di PC |
| Internet di PC | Tidak ada selama tersambung ke ESP32 | Tetap ada |
| Cocok untuk | Satu kamera, lokasi tanpa WiFi | Beberapa kamera |

### Mode AP

```cpp
const bool MODE_AP = true;
const char *AP_SSID = "Penghitung-Pengunjung";
const char *AP_PASS = "12345678";   // minimal 8 karakter
```

Di `server/server.py`, biarkan:
```python
KAMERA_AP = ["http://192.168.4.1/data"]
```

### Mode STA

```cpp
const bool MODE_AP = false;
const char *WIFI_SSID  = "NamaHotspot";
const char *WIFI_PASS  = "PasswordHotspot";
const char *SERVER_URL = "http://IP_PC:5000/api/update";
```

Di `server/server.py`, ubah menjadi `KAMERA_AP = []`.

Cara mengetahui `IP_PC`: jalankan `python server.py`, lalu lihat baris **URL di ESP32** di terminal. Bisa juga dengan perintah `ipconfig` (Windows) atau `ip addr` (Linux) dan melihat *IPv4 Address* pada adapter WiFi.

> Hotspot kampus yang memakai halaman login (*captive portal*) tidak bisa dipakai ESP32. Gunakan hotspot HP dengan pita **2,4 GHz** dan keamanan **WPA2**.

## Upload firmware

1. **Tutup Serial Monitor** terlebih dahulu, karena Serial Monitor yang terbuka akan menghalangi upload.
2. Klik **Upload**. Kompilasi pertama bisa memakan waktu 5–15 menit karena SDK Edge Impulse cukup besar.
3. Upload berhasil jika muncul `Hash of data verified` dan `Hard resetting via RTS pin`.
4. Buka **Serial Monitor** (115200 baud) dan tekan tombol **RST**.

Contoh keluaran yang benar (mode AP):
```
WiFi "Penghitung-Pengunjung" aktif (password: 12345678)
Alamat kamera   : http://192.168.4.1
Data untuk PC   : http://192.168.4.1/data
DSP: 8 ms, Inferensi: 669 ms, Deteksi: 0
```

## Jalankan server dan dashboard

```bash
cd server
python server.py
```

- Dashboard: **http://127.0.0.1:5000**
- Pratinjau deteksi: **http://192.168.4.1** (mode AP) atau IP ESP32 yang tampil di Serial Monitor (mode STA)

Pada mode AP, terminal akan menampilkan `Kamera http://192.168.4.1/data terhubung.` setelah PC tersambung ke WiFi ESP32.

## Mencoba dashboard tanpa ESP32

Jalankan `server.py`, lalu di terminal kedua:
```bash
python simulasi.py
```
Setelah selesai, hentikan simulasi (Ctrl+C), hentikan server, hapus folder `server/data`, lalu jalankan server lagi agar data simulasi tidak tercampur.

## Membaca halaman pratinjau

- **Lingkaran kuning**: orang yang terdeteksi beserta skor keyakinannya.
- **Garis merah**: garis hitung. Pita merah tipis di sekitarnya adalah zona mati.
- **Titik biru/hijau**: orang yang sedang dilacak, di atas atau di bawah garis.
- **Area gelap di kiri dan kanan**: tidak dilihat model, karena model hanya memproses bagian tengah gambar yang berbentuk persegi.

## Data

Setiap perubahan hitungan disimpan di `server/data/kunjungan_YYYY-MM-DD.csv` dengan kolom `waktu, perangkat, masuk, keluar, mentah_masuk, mentah_keluar`. Tombol **Unduh data hari ini (CSV)** di dashboard mengunduh file tersebut.
