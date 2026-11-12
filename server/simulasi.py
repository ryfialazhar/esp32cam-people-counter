"""
Uji dashboard tanpa ESP32-CAM: mensimulasikan kamera yang mengirim data.
Jalankan server.py dulu, lalu di terminal lain:  python simulasi.py
"""
import json
import random
import time
import urllib.request

URL = "http://127.0.0.1:5000/api/update"
kamera = {"pintu-utama": [0, 0], "pintu-samping": [0, 0]}

print("Simulasi berjalan. Tekan Ctrl+C untuk berhenti.")
while True:
    for nama, hitung in kamera.items():
        hitung[0] += random.choice([0, 0, 1, 1, 2])
        if hitung[1] < hitung[0]:
            hitung[1] += random.choice([0, 0, 1])
        body = json.dumps({"perangkat": nama, "masuk": hitung[0], "keluar": hitung[1]}).encode()
        req = urllib.request.Request(URL, data=body, headers={"Content-Type": "application/json"})
        try:
            urllib.request.urlopen(req, timeout=2)
        except OSError as e:
            print("Gagal kirim:", e)
    time.sleep(2)
