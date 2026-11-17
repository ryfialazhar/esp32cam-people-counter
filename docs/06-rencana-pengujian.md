# 6. Rencana Pengujian (Capstone Project)

Dokumen ini adalah panduan kerja tim untuk bagian **pengujian** proyek capstone. Hasilnya dipakai sebagai bukti kinerja alat saat presentasi lomba.

## Tujuan

1. Memastikan semua fungsi sistem berjalan sesuai rancangan.
2. Mengukur **akurasi penghitungan** pada beberapa skenario nyata.
3. Mengukur **kinerja** perangkat (waktu inferensi, laju proses, keterlambatan tampil di dashboard).
4. Menguji **ketahanan** alat saat dinyalakan lama dan saat terjadi gangguan.
5. Menentukan **nilai parameter terbaik** untuk lokasi pameran.

## Pembagian peran

| Peran | Tugas |
|---|---|
| Koordinator uji | Menyiapkan jadwal, memastikan prosedur diikuti, memeriksa kelengkapan data |
| Operator alat | Memasang kamera, menjalankan server, mereset hitungan, merekam log Serial |
| Pejalan uji (2–3 orang) | Melintas sesuai skenario |
| Pencatat | Mencatat hitungan manual dan jenis kesalahan di lembar uji |
| Dokumentasi | Merekam video setiap sesi (bukti dan sumber verifikasi), mengambil foto pemasangan |

Rekaman video dipakai untuk memeriksa ulang hitungan manual jika ada selisih. Minta persetujuan setiap orang yang terekam.

## Persiapan

- [ ] Firmware dan server berjalan sesuai [panduan 3](03-menjalankan-sistem.md).
- [ ] Kamera terpasang di pintu uji sesuai [panduan 4](04-pengujian-dan-penyetelan.md), dengan posisi dicatat: tinggi, jarak ke pintu, sudut, dan foto pemasangan.
- [ ] Nilai parameter firmware dicatat (`POSISI_GARIS`, `ZONA_MATI`, `JARAK_MAKS`, `MAKS_HILANG`, `AMBANG_KEYAKINAN`).
- [ ] Salin `pengujian/lembar_akurasi.csv` menjadi `pengujian/data/akurasi.csv`.
- [ ] Siapkan aplikasi *tally counter* di HP untuk pencatat dan kamera HP untuk dokumentasi.

## A. Pengujian fungsional

Tandai **Berhasil** atau **Gagal** dan tulis catatan untuk setiap butir.

| No | Fungsi yang diuji | Cara uji | Hasil yang diharapkan |
|---|---|---|---|
| F1 | WiFi mode AP | Nyalakan ESP32, cari WiFi di PC | `Penghitung-Pengunjung` muncul dan bisa disambungkan |
| F2 | Server terhubung ke kamera | Jalankan `server.py` | Terminal menampilkan `Kamera ... terhubung` |
| F3 | Status kamera | Buka dashboard | Kamera `pintu-utama` bertitik hijau |
| F4 | Hitung masuk | Satu orang masuk | Total masuk bertambah 1 |
| F5 | Hitung keluar | Satu orang keluar | Total keluar bertambah 1 |
| F6 | Orang di dalam ruangan | Setelah F4 dan F5 | Angka di dalam ruangan sesuai (masuk − keluar) |
| F7 | Grafik per jam | Lihat dashboard | Batang jam berjalan bertambah |
| F8 | Reset hitungan | Klik *Mulai hitungan baru* | Semua angka kembali ke 0 |
| F9 | Unduh CSV | Klik *Unduh data hari ini* | File CSV terunduh dan berisi data |
| F10 | Pratinjau deteksi | Buka http://192.168.4.1 | Gambar, lingkaran deteksi, dan garis tampil |
| F11 | ESP32 restart | Tekan RST saat hitungan sudah ada | Total di dashboard tidak hilang dan tidak dobel |
| F12 | Server restart | Hentikan lalu jalankan lagi `server.py` | Total hari itu tetap sama |
| F13 | Kamera terputus | Cabut daya ESP32 selama 30 detik | Titik kamera berubah merah, lalu hijau lagi setelah dinyalakan |

## B. Pengujian akurasi penghitungan

Setiap skenario terdiri dari **10 kali masuk dan 10 kali keluar**, diulang **3 kali**. Klik *Mulai hitungan baru* sebelum setiap ulangan.

| Kode | Skenario | Tujuan |
|---|---|---|
| S1 | 1 orang, jalan biasa | Kondisi dasar |
| S2 | 1 orang, jalan cepat | Batas kecepatan alat (~1,2 frame/detik) |
| S3 | 2 orang berdampingan | Orang berdekatan |
| S4 | 2 orang beriringan (jarak 1–2 m) | Pelacakan beberapa orang |
| S5 | 2 orang berlawanan arah bersamaan | Arah masuk dan keluar bersamaan |
| S6 | Pencahayaan redup | Pengaruh cahaya |
| S7 | Membawa tas besar atau payung | Bentuk tubuh berubah |
| S8 | 1 orang berdiri diam di dekat garis selama 30 detik | Hitungan palsu (seharusnya tetap 0) |

**Isi lembar uji** (`pengujian/data/akurasi.csv`) untuk setiap ulangan:
- `masuk_manual`, `keluar_manual`: hitungan sebenarnya (pencatat).
- `masuk_alat`, `keluar_alat`: angka di dashboard setelah ulangan selesai.
- `terlewat`: lintasan yang tidak terhitung.
- `dobel`: satu lintasan yang terhitung lebih dari sekali.
- `arah_terbalik`: masuk yang tercatat keluar atau sebaliknya.
- `catatan`: kondisi khusus (cahaya, kecepatan, kejadian tak terduga).

Kolom `terlewat`, `dobel`, dan `arah_terbalik` diisi dengan bantuan rekaman video. Untuk S8, isi `masuk_manual` dan `keluar_manual` dengan 0.

**Ukuran akurasi** per skenario:

```
akurasi = 1 − (Σ|alat − manual|) / (Σ manual)
```

## C. Pengujian kinerja

1. **Waktu inferensi.** Tutup Serial Monitor, lalu rekam log selama 5 menit:
   ```bash
   cd pengujian
   pip install pyserial
   python catat_serial.py COM3
   ```
   Skrip analisis menghitung rata-rata, simpangan baku, minimum, dan maksimum.
2. **Keterlambatan dashboard.** Dengan stopwatch, ukur waktu dari orang melewati garis sampai angka di dashboard berubah. Ulangi 10 kali dan hitung rata-ratanya.
3. **Sumber daya model.** Catat perkiraan RAM dan flash dari Edge Impulse (bagian *On-device performance*).

## D. Penyetelan parameter

Ulangi skenario **S1 dan S2** (masing-masing 1 ulangan) untuk setiap nilai:

| Parameter | Nilai yang diuji |
|---|---|
| `ZONA_MATI` | 0.03, 0.05, 0.10 |
| `JARAK_MAKS` | 0.30, 0.50, 0.70 |
| `AMBANG_KEYAKINAN` | 0.40, 0.50, 0.60 |

Ubah **satu parameter saja** setiap kali, upload ulang firmware, dan beri nama skenario yang jelas di lembar uji, misalnya `S1 ZONA_MATI=0.10`. Gunakan kombinasi terbaik untuk pengujian B final.

## E. Pengujian ketahanan

1. Nyalakan alat selama **minimal 2 jam** di lokasi dengan lalu lintas orang nyata, sambil merekam log Serial.
2. Catat: apakah ESP32 pernah restart sendiri, apakah koneksi pernah putus, dan suhu modul (rasakan dengan tangan atau termometer).
3. Bandingkan total hitungan alat dengan hitungan manual pada beberapa rentang 15 menit.

## Mengolah hasil

```bash
cd pengujian
pip install matplotlib
python analisis.py --akurasi data/akurasi.csv --serial data/serial_xxx.txt
```

Skrip menghasilkan folder `pengujian/hasil/` berisi `ringkasan.md` (tabel akurasi dan kinerja), `akurasi_per_skenario.png`, dan `waktu_inferensi.png`. Coba dulu dengan data contoh:

```bash
python analisis.py --akurasi contoh/akurasi_contoh.csv --serial contoh/serial_contoh.txt
```

## Luaran untuk presentasi lomba

- [ ] Tabel hasil pengujian fungsional (F1–F13).
- [ ] Tabel dan grafik akurasi per skenario, plus akurasi keseluruhan.
- [ ] Tabel kinerja: waktu inferensi, laju proses, keterlambatan dashboard, RAM, dan flash.
- [ ] Tabel pengaruh parameter dan nilai terbaik yang dipilih.
- [ ] Hasil uji ketahanan.
- [ ] Video demo singkat (1–2 menit): kamera, orang melintas, angka di dashboard bertambah.
- [ ] Foto pemasangan alat dan tangkapan layar dashboard serta pratinjau.
- [ ] Analisis kesalahan: skenario mana yang paling sering salah, penyebabnya, dan rencana perbaikan.

Simpan semua data mentah di `pengujian/data/` dan hasil olahan di `pengujian/hasil/`, lalu commit ke repositori agar bisa ditelusuri juri atau pembimbing.
