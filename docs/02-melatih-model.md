# 2. Melatih Model di Edge Impulse

Model yang dipakai adalah **FOMO (Faster Objects, More Objects)**, model deteksi objek ringan yang menghasilkan titik pusat setiap objek. FOMO cocok untuk menghitung karena ringan dan bisa mendeteksi beberapa orang sekaligus.

## A. Clone proyek publik

1. Buat akun dan login di **Edge Impulse Studio** (studio.edgeimpulse.com).
2. Buka **https://www.edgeimpulse.com/projects**, lalu cari `person detection`.
3. Buka proyek **Person Detection with Renesas DRP-AI** (oleh Edge Impulse Experts). Proyek ini berjenis *Object Detection* dengan label `person` dan sekitar 744 gambar.
4. Klik **Clone this project**. Jika kuota proyek privat habis, jadikan hasil clone sebagai *public*.

> Model asli proyek tersebut dibuat untuk Renesas RZ/V2L (sekitar 45 MB) sehingga **tidak muat** di ESP32. Yang dipakai hanya dataset-nya; modelnya diganti ke FOMO pada langkah berikut.

## B. Atur impulse

1. Buka menu ☰ > **Impulse design** > **Create impulse**.
2. *Image data*: ukuran **96 x 96**, *Resize mode*: **Fit shortest axis**.
3. Processing block: **Image**. Learning block: **Object Detection (Images)**. Klik **Save Impulse**.
4. Di menu **Image**, ubah *Color depth* menjadi **Grayscale**, klik **Save parameters**, lalu **Generate features**.

## C. Latih model

1. Di menu **Object detection**, klik **Choose a different model** dan pilih **FOMO (Faster Objects, More Objects) MobileNetV2 0.35**.
2. Pengaturan yang dipakai pada percobaan awal: *Number of training cycles* **100**, *Learning rate* **0.001**, *Data augmentation* **dicentang**.
3. Klik **Save & train**.
4. Setelah selesai, ubah *Model version* menjadi **Quantized (int8)** dan catat F1 score, precision, dan recall.

## D. Atur target perangkat

1. Klik ikon chip di kanan atas Studio.
2. Pilih target **Espressif ESP-EYE (ESP32 240MHz)**, chip yang sama dengan ESP32-CAM.
3. Atur *Application budget*: RAM **4 MB**, ROM **3 MB**, Latency **300 ms**. Klik **Save**.
4. Lihat bagian **On-device performance** untuk perkiraan waktu inferensi, RAM, dan flash.

> Perkiraan Edge Impulse untuk ESP32 bisa lebih lambat dari kenyataan. Pada percobaan awal, perkiraannya 987 ms, sedangkan di perangkat terukur sekitar 670 ms karena library Arduino memakai optimasi ESP-NN.

## E. Ekspor sebagai Arduino library

1. Buka menu **Deployment**, pilih **Arduino library**.
2. Pilih **Quantized (int8)** dan **EON Compiler**, lalu klik **Build**. File ZIP akan terunduh.
3. Di Arduino IDE: **Sketch > Include Library > Add .ZIP Library**, lalu pilih file ZIP tersebut.

## F. Sesuaikan firmware

Buka `firmware/esp32_penghitung/esp32_penghitung.ino`, lalu sesuaikan dua hal:

```cpp
// Nama header mengikuti nama proyek Edge Impulse Anda, contoh:
#include <Person_Detection_with_Renesas_DRP-AI_inferencing.h>

// Label harus sama dengan label di Edge Impulse
const char *LABEL_ORANG = "person";
```

Nama header bisa dilihat di folder `src` di dalam library yang sudah dipasang (`Documents/Arduino/libraries/...`).

## Cek pemahaman

- [ ] Mengapa model asli proyek perlu diganti ke FOMO?
- [ ] Apa beda versi float32 dan int8, dan mengapa int8 dipakai di ESP32?
- [ ] Berapa F1, precision, dan recall model Anda? Mana yang paling penting untuk penghitung pengunjung, dan mengapa?
