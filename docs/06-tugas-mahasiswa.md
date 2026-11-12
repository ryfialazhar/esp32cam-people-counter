# 6. Tugas Mahasiswa

Tugas disusun bertingkat. Setiap kelompok mengumpulkan laporan singkat berisi langkah yang dilakukan, hasil (tabel/grafik), dan analisis.

## Tingkat 1: Menjalankan sistem

1. Jalankan `server.py` dan `simulasi.py`, lalu jelaskan alur data dari `simulasi.py` sampai tampil di dashboard.
2. Clone dan latih model sesuai [panduan 2](02-melatih-model.md). Laporkan F1, precision, recall (float32 dan int8), serta perkiraan waktu inferensi.
3. Upload firmware dan jalankan sistem dalam mode AP. Lampirkan screenshot Serial Monitor, halaman pratinjau, dan dashboard.

## Tingkat 2: Pengujian dan penyetelan

4. Lakukan prosedur uji akurasi pada [panduan 4](04-pengujian-dan-penyetelan.md) untuk minimal tiga skenario.
5. Pilih dua parameter (misalnya `ZONA_MATI` dan `JARAK_MAKS`). Uji minimal tiga nilai untuk masing-masing, lalu sajikan pengaruhnya terhadap akurasi dalam tabel.
6. Bandingkan dua posisi pemasangan kamera (misalnya tinggi 1,5 m dan 2,5 m). Jelaskan mengapa hasilnya berbeda.

## Tingkat 3: Pengembangan

Pilih minimal satu:

7. **Model lebih cepat.** Latih FOMO MobileNetV2 0.1 dan bandingkan dengan 0.35 dari sisi F1, waktu inferensi, dan akurasi hitung di lokasi.
8. **Dataset lokal.** Kumpulkan 100 foto dari ESP32-CAM di lokasi pemasangan, beri label di Edge Impulse, latih ulang, lalu bandingkan akurasinya dengan model awal.
9. **Beberapa kamera.** Jalankan dua ESP32-CAM dalam mode STA dengan `ID_PERANGKAT` berbeda, lalu tampilkan keduanya di dashboard.
10. **Notifikasi kapasitas.** Tambahkan batas maksimal orang di dalam ruangan. Dashboard berubah warna dan menampilkan peringatan saat batas terlampaui.
11. **Laporan harian.** Buat skrip Python yang membaca CSV dan menghasilkan grafik serta ringkasan (total pengunjung, jam tersibuk, puncak jumlah orang di dalam ruangan).
12. **Kamera dari atas (top-down).** Kumpulkan dataset dari sudut atas, latih model baru, dan bandingkan dengan sudut miring.

## Pertanyaan diskusi

- Mengapa recall lebih penting daripada precision untuk penghitung pengunjung? Kapan sebaliknya?
- Apa kelebihan dan kekurangan memproses AI di perangkat (*edge*) dibanding mengirim video ke server?
- Bagaimana menjaga privasi pengunjung pada sistem berbasis kamera seperti ini?

## Rubrik penilaian (saran)

| Aspek | Bobot |
|---|---|
| Sistem berjalan dan terdokumentasi (Tingkat 1) | 30% |
| Kualitas pengujian dan analisis (Tingkat 2) | 35% |
| Pengembangan dan kebaruan (Tingkat 3) | 25% |
| Kerapian laporan dan presentasi | 10% |
