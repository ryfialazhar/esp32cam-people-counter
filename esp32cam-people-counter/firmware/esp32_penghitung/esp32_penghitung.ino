/*
 * Penghitung Pengunjung Pameran
 * ESP32-CAM (AI Thinker) + Edge Impulse FOMO
 *
 * Kamera dipasang di atas pintu menghadap ke bawah. Setiap orang yang
 * melewati garis virtual di tengah frame dihitung masuk atau keluar,
 * lalu jumlahnya dikirim ke PC (server.py) lewat WiFi.
 *
 * Pengaturan board di Arduino IDE:
 *   Board   : AI Thinker ESP32-CAM
 *   PSRAM   : Enabled
 *   Paket   : esp32 by Espressif Systems versi 2.0.17 (jangan 3.x)
 *
 * Repositori: https://github.com/ryfialazhar/esp32cam-people-counter
 * Lisensi   : MIT
 */

// Library hasil ekspor Edge Impulse (FOMO 96x96 grayscale, int8).
// GANTI nama header ini sesuai nama proyek Edge Impulse Anda (lihat docs/02-melatih-model.md).
#include <Person_Detection_with_Renesas_DRP-AI_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

#if !defined(EI_CLASSIFIER_OBJECT_DETECTION) || (EI_CLASSIFIER_OBJECT_DETECTION != 1)
#error "Model harus berjenis Object Detection (FOMO)."
#endif

// ================== KONFIGURASI ==================
// MODE_AP = true : ESP32 membuat WiFi sendiri. Sambungkan PC/HP ke WiFi ini.
//                  server.py mengambil data dari http://192.168.4.1/data.
// MODE_AP = false: ESP32 menyambung ke WiFi/hotspot yang ada dan mengirim data ke PC.
const bool MODE_AP = true;
const char *AP_SSID = "Penghitung-Pengunjung";
const char *AP_PASS = "12345678";   // minimal 8 karakter

// Dipakai hanya jika MODE_AP = false
const char *WIFI_SSID   = "NamaHotspot";
const char *WIFI_PASS   = "PasswordHotspot";
const char *SERVER_URL  = "http://192.168.1.10:5000/api/update";  // IP PC yang menjalankan server.py
const char *ID_PERANGKAT = "pintu-utama";  // beri nama berbeda untuk tiap kamera

const char *LABEL_ORANG = "person";  // label dari model Edge Impulse
const float AMBANG_KEYAKINAN = 0.50; // skor minimal deteksi (0..1)

const bool  MASUK_KE_BAWAH = true;   // true: bergerak dari atas frame ke bawah = masuk
const float POSISI_GARIS   = 0.50;   // posisi garis, 0 = atas frame, 1 = bawah frame
const float ZONA_MATI      = 0.05;   // zona di sekitar garis agar tidak terhitung dobel
const float JARAK_MAKS     = 0.50;   // jarak maksimal antar-frame untuk orang yang sama (proporsi lebar)
const uint8_t MAKS_HILANG  = 4;      // jumlah frame tanpa deteksi sebelum jejak dihapus
const unsigned long INTERVAL_LAPOR = 5000;  // kirim status rutin tiap 5 detik

// Halaman pratinjau deteksi di http://IP_ESP32/ (untuk pemasangan dan uji coba).
// Ubah ke false saat pameran jika tidak ingin gambar kamera bisa dibuka.
const bool AKTIFKAN_PRATINJAU = true;
// =================================================

// Pin kamera AI Thinker
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

#define LEBAR_KAMERA  320
#define TINGGI_KAMERA 240

static uint8_t *snapshot_buf = nullptr;

// Data untuk halaman pratinjau
WebServer web(80);
#define MAKS_JPEG 60000
#define MAKS_KOTAK 12
struct Kotak { float x, y, w, h, skor; };
uint8_t *jpegTerakhir = nullptr;
size_t jpegPanjang = 0;
Kotak kotakTerakhir[MAKS_KOTAK];
int jumlahKotak = 0;
uint32_t waktuInferensi = 0;

// ---------- Kamera ----------
bool kameraMulai() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;   c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;   c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = FRAMESIZE_QVGA;   // 320x240
  c.jpeg_quality = 12;
  c.fb_count     = 1;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  if (esp_camera_init(&c) != ESP_OK) return false;

  sensor_t *s = esp_camera_sensor_get();
  // Aktifkan jika gambar terbalik sesuai posisi pemasangan
  // s->set_vflip(s, 1);
  // s->set_hmirror(s, 1);
  (void)s;
  return true;
}

bool ambilGambar(uint32_t lebar, uint32_t tinggi, uint8_t *buf) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return false;
  if (AKTIFKAN_PRATINJAU && jpegTerakhir && fb->len <= MAKS_JPEG) {
    memcpy(jpegTerakhir, fb->buf, fb->len);  // simpan salinan untuk halaman pratinjau
    jpegPanjang = fb->len;
  }
  bool ok = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, buf);
  esp_camera_fb_return(fb);
  if (!ok) return false;
  if (lebar != LEBAR_KAMERA || tinggi != TINGGI_KAMERA) {
    ei::image::processing::crop_and_interpolate_rgb888(
        buf, LEBAR_KAMERA, TINGGI_KAMERA, buf, lebar, tinggi);
  }
  return true;
}

static int ambilData(size_t offset, size_t panjang, float *keluaran) {
  size_t ix = offset * 3;
  for (size_t i = 0; i < panjang; i++, ix += 3) {
    // fmt2rgb888 menghasilkan urutan BGR
    keluaran[i] = (snapshot_buf[ix + 2] << 16) + (snapshot_buf[ix + 1] << 8) + snapshot_buf[ix];
  }
  return 0;
}

// ---------- Pelacakan & penghitungan ----------
struct Jejak {
  bool aktif;
  float x, y;
  int8_t sisi;     // -1 di atas garis, 1 di bawah garis, 0 belum diketahui
  uint8_t hilang;
};

#define MAKS_JEJAK 12
Jejak jejak[MAKS_JEJAK];
uint32_t jumlahMasuk = 0, jumlahKeluar = 0;
bool adaPerubahan = false;

int8_t tentukanSisi(float y, int8_t sisiLama) {
  const float H = EI_CLASSIFIER_INPUT_HEIGHT;
  float garis = POSISI_GARIS * H, zona = ZONA_MATI * H;
  if (y < garis - zona) return -1;
  if (y > garis + zona) return 1;
  return sisiLama;  // masih di zona mati: pertahankan sisi sebelumnya
}

void perbaruiJejak(const float *dx, const float *dy, int n) {
  bool dipakai[MAKS_JEJAK] = {false};
  const float jarakMaks = JARAK_MAKS * EI_CLASSIFIER_INPUT_WIDTH;

  for (int d = 0; d < n; d++) {
    int terbaik = -1;
    float jarakTerbaik = jarakMaks;
    for (int t = 0; t < MAKS_JEJAK; t++) {
      if (!jejak[t].aktif || dipakai[t]) continue;
      float j = hypotf(jejak[t].x - dx[d], jejak[t].y - dy[d]);
      if (j < jarakTerbaik) { jarakTerbaik = j; terbaik = t; }
    }

    if (terbaik >= 0) {
      Jejak &jk = jejak[terbaik];
      int8_t sisiBaru = tentukanSisi(dy[d], jk.sisi);
      if (jk.sisi != 0 && sisiBaru != jk.sisi) {
        bool keBawah = (sisiBaru == 1);
        if (keBawah == MASUK_KE_BAWAH) jumlahMasuk++; else jumlahKeluar++;
        adaPerubahan = true;
        Serial.printf("Masuk: %lu  Keluar: %lu\n", (unsigned long)jumlahMasuk, (unsigned long)jumlahKeluar);
      }
      jk.x = dx[d]; jk.y = dy[d]; jk.sisi = sisiBaru; jk.hilang = 0;
      dipakai[terbaik] = true;
    } else {
      for (int t = 0; t < MAKS_JEJAK; t++) {
        if (!jejak[t].aktif) {
          jejak[t].aktif = true;
          jejak[t].x = dx[d]; jejak[t].y = dy[d];
          jejak[t].sisi = tentukanSisi(dy[d], 0);
          jejak[t].hilang = 0;
          dipakai[t] = true;
          break;
        }
      }
    }
  }

  for (int t = 0; t < MAKS_JEJAK; t++) {
    if (jejak[t].aktif && !dipakai[t] && ++jejak[t].hilang > MAKS_HILANG) {
      jejak[t].aktif = false;
    }
  }
}

// ---------- Halaman pratinjau ----------
const char HALAMAN[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="id"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pratinjau Deteksi</title>
<style>
body{margin:0;padding:16px;background:#0e1b28;color:#e8edf1;font:15px system-ui,sans-serif;display:flex;flex-direction:column;align-items:center}
h2{margin:4px 0 12px}
canvas{width:min(100%,640px);height:auto;border-radius:8px;background:#000}
.info{display:flex;gap:28px;margin:14px 0;flex-wrap:wrap;justify-content:center;text-align:center}
.info b{display:block;font-size:1.5em}
small{color:#9aabb9;max-width:640px;line-height:1.6;text-align:center}
</style></head><body>
<h2>Pratinjau Deteksi</h2>
<canvas id="c" width="320" height="240"></canvas>
<div class="info">
<div><b id="m">0</b>Masuk</div><div><b id="k">0</b>Keluar</div>
<div><b id="d">0</b>Orang terdeteksi</div><div><b id="t">-</b>Inferensi (ms)</div>
</div>
<small>Lingkaran kuning: orang yang terdeteksi beserta skornya. Garis merah: garis hitung, dan pita merah tipis di sekitarnya adalah zona mati.
Titik biru: orang yang dilacak di atas garis, titik hijau: di bawah garis. Bagian gelap di tepi tidak dilihat model. Gambar diperbarui setiap kali model selesai memproses satu frame.</small>
<script>
const $=id=>document.getElementById(id),c=$('c'),g=c.getContext('2d'),img=new Image();
let data=null;
img.onload=gambar;
let urlLama=null;
async function perbarui(){
  try{
    const r=await fetch('/foto.jpg',{cache:'no-store'});
    if(r.ok){
      data=JSON.parse(r.headers.get('X-Deteksi'));
      $('m').textContent=data.masuk;$('k').textContent=data.keluar;
      $('d').textContent=data.kotak.length;$('t').textContent=data.inferensi;
      const url=URL.createObjectURL(await r.blob());
      img.src=url;
      if(urlLama)URL.revokeObjectURL(urlLama);
      urlLama=url;
    }
  }catch(e){await new Promise(t=>setTimeout(t,1000));}
  setTimeout(perbarui,50);
}
function gambar(){
  const W=img.naturalWidth,H=img.naturalHeight;
  c.width=W;c.height=H;g.drawImage(img,0,0);
  if(!data)return;
  let cw,ch;
  if(W/H>data.iw/data.ih){ch=H;cw=H*data.iw/data.ih}else{cw=W;ch=W*data.ih/data.iw}
  const ox=(W-cw)/2,oy=(H-ch)/2,s=cw/data.iw;
  g.fillStyle='rgba(0,0,0,.55)';
  g.fillRect(0,0,ox,H);g.fillRect(W-ox,0,ox,H);g.fillRect(0,0,W,oy);g.fillRect(0,H-oy,W,oy);
  const gy=oy+data.garis*ch,z=data.zona*ch;
  g.fillStyle='rgba(217,84,63,.2)';g.fillRect(ox,gy-z,cw,2*z);
  g.strokeStyle='#d9543f';g.lineWidth=2;g.beginPath();g.moveTo(ox,gy);g.lineTo(ox+cw,gy);g.stroke();
  g.font='bold 11px sans-serif';g.textAlign='center';
  data.kotak.forEach(b=>{
    const cx=ox+(b.x+b.w/2)*s,cy=oy+(b.y+b.h/2)*s;
    g.strokeStyle='#ffc531';g.lineWidth=2;g.beginPath();g.arc(cx,cy,10,0,7);g.stroke();
    g.fillStyle='#ffc531';g.fillText(Math.round(b.skor*100)+'%',cx,cy-14);
  });
  data.jejak.forEach(j=>{
    g.fillStyle=j.sisi<0?'#4fc3f7':(j.sisi>0?'#1f9d6a':'#cccccc');
    g.beginPath();g.arc(ox+j.x*s,oy+j.y*s,4,0,7);g.fill();
  });
}
perbarui();
</script></body></html>)rawliteral";

void kirimHalaman() {
  web.send_P(200, "text/html", HALAMAN);
}

String dataDeteksi();

void kirimFoto() {
  if (jpegPanjang == 0) { web.send(503, "text/plain", "Belum ada gambar"); return; }
  web.sendHeader("Cache-Control", "no-store");
  web.sendHeader("X-Deteksi", dataDeteksi());
  web.setContentLength(jpegPanjang);
  web.send(200, "image/jpeg", "");
  web.client().write(jpegTerakhir, jpegPanjang);
}

String dataDeteksi() {
  String j;
  j.reserve(1024);
  j += "{\"perangkat\":\""; j += ID_PERANGKAT; j += "\"";
  j += ",\"masuk\":";     j += jumlahMasuk;
  j += ",\"keluar\":";    j += jumlahKeluar;
  j += ",\"inferensi\":"; j += waktuInferensi;
  j += ",\"iw\":";        j += EI_CLASSIFIER_INPUT_WIDTH;
  j += ",\"ih\":";        j += EI_CLASSIFIER_INPUT_HEIGHT;
  j += ",\"garis\":";     j += String(POSISI_GARIS, 3);
  j += ",\"zona\":";      j += String(ZONA_MATI, 3);
  j += ",\"kotak\":[";
  for (int i = 0; i < jumlahKotak; i++) {
    const Kotak &b = kotakTerakhir[i];
    if (i) j += ",";
    j += "{\"x\":"; j += String(b.x, 1); j += ",\"y\":"; j += String(b.y, 1);
    j += ",\"w\":"; j += String(b.w, 1); j += ",\"h\":"; j += String(b.h, 1);
    j += ",\"skor\":"; j += String(b.skor, 2); j += "}";
  }
  j += "],\"jejak\":[";
  bool pertama = true;
  for (int t = 0; t < MAKS_JEJAK; t++) {
    if (!jejak[t].aktif) continue;
    if (!pertama) j += ",";
    pertama = false;
    j += "{\"x\":"; j += String(jejak[t].x, 1); j += ",\"y\":"; j += String(jejak[t].y, 1);
    j += ",\"sisi\":"; j += jejak[t].sisi; j += "}";
  }
  j += "]}";
  return j;
}

void kirimData() {
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json", dataDeteksi());
}

// ---------- Jaringan ----------
unsigned long terakhirLapor = 0, terakhirCobaWifi = 0;

void laporKeServer() {
  terakhirLapor = millis();
  if (WiFi.status() != WL_CONNECTED) return;

  char body[160];
  snprintf(body, sizeof(body),
           "{\"perangkat\":\"%s\",\"masuk\":%lu,\"keluar\":%lu}",
           ID_PERANGKAT, (unsigned long)jumlahMasuk, (unsigned long)jumlahKeluar);

  HTTPClient http;
  http.setTimeout(1500);
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  int kode = http.POST((uint8_t *)body, strlen(body));
  http.end();

  if (kode == 200) adaPerubahan = false;
  else Serial.printf("Gagal kirim ke server (kode %d)\n", kode);
}

bool sudahTerhubung = false;

const char *artiStatusWifi(wl_status_t st) {
  switch (st) {
    case WL_NO_SSID_AVAIL:  return "nama WiFi tidak ditemukan (cek nama, pita 2,4 GHz, jarak)";
    case WL_CONNECT_FAILED: return "gagal terhubung (cek password atau keamanan WPA2)";
    case WL_CONNECTION_LOST:return "koneksi hilang";
    case WL_DISCONNECTED:   return "belum terhubung";
    case WL_IDLE_STATUS:    return "sedang mencoba";
    default:                return "status lain";
  }
}

// Menampilkan daftar WiFi yang terlihat oleh ESP32, untuk diagnosis
void pindaiWifi() {
  Serial.println("Memindai jaringan WiFi di sekitar...");
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    Serial.println("  Tidak ada jaringan WiFi yang terlihat. Periksa antena atau jarak.");
  }
  for (int i = 0; i < n; i++) {
    Serial.printf("  %2d. \"%s\"  sinyal %d dBm  kanal %d%s\n", i + 1,
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                  WiFi.SSID(i) == WIFI_SSID ? "  <-- ini yang dicari" : "");
  }
  WiFi.scanDelete();
}

void jagaWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!sudahTerhubung) {
      sudahTerhubung = true;
      Serial.print("Terhubung, IP: ");
      Serial.println(WiFi.localIP());
      if (AKTIFKAN_PRATINJAU) {
        Serial.print("Pratinjau deteksi: http://");
        Serial.println(WiFi.localIP());
      }
    }
    return;
  }
  sudahTerhubung = false;
  if (millis() - terakhirCobaWifi < 10000) return;
  terakhirCobaWifi = millis();
  wl_status_t st = WiFi.status();
  Serial.printf("WiFi belum terhubung (kode %d: %s), mencoba lagi...\n", st, artiStatusWifi(st));
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// ---------- Program utama ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  if (!kameraMulai()) {
    Serial.println("Kamera gagal dinyalakan. Periksa kabel dan daya 5V 2A.");
    while (true) delay(1000);
  }

  snapshot_buf = (uint8_t *)ps_malloc(LEBAR_KAMERA * TINGGI_KAMERA * 3);
  if (!snapshot_buf) {
    Serial.println("PSRAM tidak tersedia. Pastikan opsi PSRAM: Enabled.");
    while (true) delay(1000);
  }

  if (MODE_AP) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("WiFi \"%s\" aktif (password: %s)\n", AP_SSID, AP_PASS);
    Serial.print("Alamat kamera   : http://");
    Serial.println(WiFi.softAPIP());
    Serial.print("Data untuk PC   : http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/data");
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Menyambung WiFi");
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
      wl_status_t st = WiFi.status();
      Serial.printf("Belum terhubung (kode %d: %s). Penghitungan tetap berjalan.\n", st, artiStatusWifi(st));
      pindaiWifi();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  if (AKTIFKAN_PRATINJAU) {
    jpegTerakhir = (uint8_t *)ps_malloc(MAKS_JPEG);
    web.on("/", kirimHalaman);
    web.on("/foto.jpg", kirimFoto);
  }
  web.on("/data", kirimData);
  web.begin();
}

void loop() {
  if (!MODE_AP) jagaWifi();
  web.handleClient();

  if (!ambilGambar(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
    delay(50);
    return;
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ambilData;

  ei_impulse_result_t hasil = {0};
  if (run_classifier(&signal, &hasil, false) != EI_IMPULSE_OK) return;
  waktuInferensi = hasil.timing.classification;
  Serial.printf("DSP: %d ms, Inferensi: %d ms, Deteksi: %u\n",
                hasil.timing.dsp, hasil.timing.classification, (unsigned)hasil.bounding_boxes_count);

  float dx[MAKS_JEJAK], dy[MAKS_JEJAK];
  int n = 0;
  for (uint32_t i = 0; i < hasil.bounding_boxes_count && n < MAKS_JEJAK; i++) {
    const auto &bb = hasil.bounding_boxes[i];
    if (bb.value < AMBANG_KEYAKINAN) continue;
    if (strcmp(bb.label, LABEL_ORANG) != 0) continue;
    dx[n] = bb.x + bb.width / 2.0f;
    dy[n] = bb.y + bb.height / 2.0f;
    if (n < MAKS_KOTAK) kotakTerakhir[n] = {(float)bb.x, (float)bb.y, (float)bb.width, (float)bb.height, bb.value};
    n++;
  }
  jumlahKotak = (n < MAKS_KOTAK) ? n : MAKS_KOTAK;

  perbaruiJejak(dx, dy, n);

  unsigned long sejakLapor = millis() - terakhirLapor;
  if (!MODE_AP && ((adaPerubahan && sejakLapor >= 1000) || sejakLapor >= INTERVAL_LAPOR)) {
    laporKeServer();
  }
}
