# 4. Pengujian dan Penyetelan

## Memasang kamera

Model dilatih dengan gambar orang **berdiri utuh dan agak jauh dari kamera**, mirip sudut CCTV. Karena itu:

- Pasang kamera setinggi **sekitar 2–3 meter**, miring ke arah pintu atau lorong.
- Pastikan orang yang lewat terlihat **utuh** dan berada di **area terang** (tengah) pada halaman pratinjau.
- Pastikan jalur orang berjalan **memotong garis merah** dari atas ke bawah gambar atau sebaliknya.
- Jangan arahkan garis hitung ke area tempat orang duduk atau berdiri lama, karena gerakan kecil di sekitar garis bisa terhitung.

## Prosedur uji akurasi

1. Klik **Mulai hitungan baru** di dashboard.
2. Satu orang berjalan **masuk 10 kali** dan **keluar 10 kali** dengan kecepatan biasa. Orang lain mencatat hitungan manual.
3. Ulangi dengan variasi: jalan cepat, dua orang berdampingan, pencahayaan berbeda.
4. Catat hasilnya:

| Skenario | Masuk (manual) | Masuk (alat) | Keluar (manual) | Keluar (alat) | Akurasi |
|---|---|---|---|---|---|
| Jalan biasa, 1 orang | 10 | | 10 | | |
| Jalan cepat, 1 orang | 10 | | 10 | | |
| 2 orang berdampingan | 10 | | 10 | | |

Akurasi sederhana dapat dihitung sebagai `1 − |alat − manual| / manual`. Catat juga jenis kesalahannya: orang terlewat, terhitung dua kali, atau arah terbalik.

## Parameter yang bisa disetel

Semua parameter ada di bagian **KONFIGURASI** pada firmware.

| Parameter | Bawaan | Fungsi | Gejala dan perbaikan |
|---|---|---|---|
| `MASUK_KE_BAWAH` | `true` | Arah yang dihitung sebagai masuk | Masuk terhitung keluar → ubah ke `false` |
| `POSISI_GARIS` | `0.50` | Posisi garis (0 atas, 1 bawah) | Geser agar garis berada di jalur jalan |
| `ZONA_MATI` | `0.05` | Lebar zona di sekitar garis | Terhitung dua kali → perbesar. Sering terlewat → perkecil |
| `JARAK_MAKS` | `0.50` | Jarak maksimal antar-frame untuk orang yang sama | Orang cepat tidak terhitung → perbesar |
| `MAKS_HILANG` | `4` | Frame tanpa deteksi sebelum jejak dihapus | Deteksi putus-putus → perbesar |
| `AMBANG_KEYAKINAN` | `0.50` | Skor minimal deteksi | Banyak deteksi palsu → naikkan. Banyak terlewat → turunkan |
| `AKTIFKAN_PRATINJAU` | `true` | Halaman gambar kamera | Matikan saat acara sungguhan |

## Keterbatasan yang diketahui

- Dengan inferensi sekitar 670 ms, alat memproses sekitar 1,2 frame per detik. Orang yang berlari bisa terlewat.
- Orang yang berjalan berdempetan bisa terdeteksi sebagai satu orang.
- Dataset bawaan berasal dari berbagai sudut kamera; akurasi akan lebih baik setelah ditambah foto dari lokasi pemasangan.
