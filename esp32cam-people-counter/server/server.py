"""
Server dashboard penghitung pengunjung pameran.

Jalankan di PC:   pip install flask
                  python server.py
Lalu buka http://localhost:5000 di browser.

ESP32-CAM mengirim total hitungan sejak menyala. Server menghitung
selisihnya, sehingga data tetap benar walau ESP32 restart atau ada
kiriman yang hilang. Semua kejadian disimpan ke data/kunjungan_TANGGAL.csv.
"""
import csv
import json
import os
import socket
import threading
import time
import urllib.request
from collections import defaultdict
from datetime import datetime

from flask import Flask, abort, jsonify, request, send_file

FOLDER = os.path.dirname(os.path.abspath(__file__))
FOLDER_DATA = os.path.join(FOLDER, "data")
BATAS_ONLINE = 15  # detik tanpa kiriman sebelum kamera dianggap offline
PENANDA_RESET = "__RESET__"

# Kamera dalam mode WiFi sendiri (MODE_AP = true): server mengambil datanya.
# PC harus tersambung ke WiFi "Penghitung-Pengunjung" milik ESP32.
# Kosongkan daftar ini ([]) jika ESP32 memakai MODE_AP = false.
KAMERA_AP = ["http://192.168.4.1/data"]
INTERVAL_AMBIL = 2  # detik
KOLOM = ["waktu", "perangkat", "masuk", "keluar", "mentah_masuk", "mentah_keluar"]

app = Flask(__name__)
kunci = threading.Lock()


class Status:
    def __init__(self):
        self.tanggal = None
        self.mentah = {}         # nilai terakhir yang dilaporkan tiap perangkat
        self.terakhir_dilihat = {}
        self.kosongkan()

    def kosongkan(self):
        self.masuk = 0
        self.keluar = 0
        self.per_jam = defaultdict(lambda: [0, 0])
        self.per_perangkat = defaultdict(lambda: [0, 0])

    def tambah(self, perangkat, dm, dk, jam):
        self.masuk += dm
        self.keluar += dk
        self.per_jam[jam][0] += dm
        self.per_jam[jam][1] += dk
        self.per_perangkat[perangkat][0] += dm
        self.per_perangkat[perangkat][1] += dk


status = Status()


def berkas_hari_ini():
    return os.path.join(FOLDER_DATA, f"kunjungan_{datetime.now():%Y-%m-%d}.csv")


def catat(baris):
    os.makedirs(FOLDER_DATA, exist_ok=True)
    path = berkas_hari_ini()
    baru = not os.path.exists(path)
    with open(path, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if baru:
            w.writerow(KOLOM)
        w.writerow(baris)


def muat_hari_ini():
    """Bangun ulang hitungan dari CSV hari ini, supaya aman bila server restart."""
    status.tanggal = datetime.now().date()
    status.kosongkan()
    status.mentah.clear()
    path = berkas_hari_ini()
    if not os.path.exists(path):
        return
    with open(path, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if r["perangkat"] == PENANDA_RESET:
                status.kosongkan()
                continue
            jam = int(r["waktu"][11:13])
            status.tambah(r["perangkat"], int(r["masuk"]), int(r["keluar"]), jam)
            status.mentah[r["perangkat"]] = (int(r["mentah_masuk"]), int(r["mentah_keluar"]))


def cek_ganti_hari():
    if status.tanggal != datetime.now().date():
        muat_hari_ini()


@app.post("/api/update")
def update():
    d = request.get_json(silent=True) or {}
    perangkat = str(d.get("perangkat", "tanpa-nama"))[:40]
    try:
        m, k = int(d.get("masuk", 0)), int(d.get("keluar", 0))
    except (TypeError, ValueError):
        return jsonify(ok=False, pesan="masuk dan keluar harus angka"), 400

    proses_laporan(perangkat, m, k)
    return jsonify(ok=True)


def proses_laporan(perangkat, m, k):
    with kunci:
        cek_ganti_hari()
        lama_m, lama_k = status.mentah.get(perangkat, (0, 0))
        if m < lama_m or k < lama_k:
            lama_m, lama_k = 0, 0  # ESP32 baru restart, hitungannya mulai dari nol
        dm, dk = m - lama_m, k - lama_k
        status.mentah[perangkat] = (m, k)
        status.terakhir_dilihat[perangkat] = time.time()

        if dm or dk:
            sekarang = datetime.now()
            status.tambah(perangkat, dm, dk, sekarang.hour)
            catat([sekarang.strftime("%Y-%m-%d %H:%M:%S"), perangkat, dm, dk, m, k])
            print(f"[{sekarang:%H:%M:%S}] {perangkat}: +{dm} masuk, +{dk} keluar")


def ambil_dari_kamera():
    """Mengambil hitungan dari kamera yang memakai mode WiFi sendiri."""
    terhubung = {}
    while True:
        for url in KAMERA_AP:
            try:
                with urllib.request.urlopen(url, timeout=3) as r:
                    d = json.loads(r.read().decode())
                proses_laporan(str(d.get("perangkat", url))[:40], int(d["masuk"]), int(d["keluar"]))
                if not terhubung.get(url):
                    print(f"Kamera {url} terhubung.")
                terhubung[url] = True
            except (OSError, ValueError, KeyError):
                if terhubung.get(url, True):
                    print(f"Kamera {url} belum bisa dihubungi. Pastikan PC tersambung ke WiFi ESP32.")
                terhubung[url] = False
        time.sleep(INTERVAL_AMBIL)


@app.get("/api/stats")
def stats():
    with kunci:
        cek_ganti_hari()
        now = time.time()
        semua_perangkat = set(status.terakhir_dilihat) | set(status.per_perangkat)
        perangkat = []
        for pid in sorted(semua_perangkat):
            dilihat = status.terakhir_dilihat.get(pid)
            m, k = status.per_perangkat.get(pid, (0, 0))
            perangkat.append({
                "id": pid,
                "masuk": m,
                "keluar": k,
                "online": dilihat is not None and now - dilihat < BATAS_ONLINE,
                "detik_lalu": None if dilihat is None else int(now - dilihat),
            })
        return jsonify(
            tanggal=status.tanggal.isoformat(),
            masuk=status.masuk,
            keluar=status.keluar,
            di_dalam=max(0, status.masuk - status.keluar),
            per_jam=[{"jam": h, "masuk": status.per_jam[h][0], "keluar": status.per_jam[h][1]}
                     for h in range(24)],
            perangkat=perangkat,
        )


@app.post("/api/reset")
def reset():
    with kunci:
        cek_ganti_hari()
        status.kosongkan()
        catat([datetime.now().strftime("%Y-%m-%d %H:%M:%S"), PENANDA_RESET, 0, 0, 0, 0])
    return jsonify(ok=True)


@app.get("/unduh")
def unduh():
    path = berkas_hari_ini()
    if not os.path.exists(path):
        abort(404, "Belum ada data hari ini.")
    return send_file(path, as_attachment=True)


@app.get("/")
def dashboard():
    return send_file(os.path.join(FOLDER, "dashboard.html"))


def ip_lokal():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return "127.0.0.1"


if __name__ == "__main__":
    muat_hari_ini()
    if KAMERA_AP:
        threading.Thread(target=ambil_dari_kamera, daemon=True).start()
    ip = ip_lokal()
    print("=" * 56)
    print(f" Dashboard   : http://localhost:5000")
    print(f" URL di ESP32: http://{ip}:5000/api/update")
    print("=" * 56)
    app.run(host="0.0.0.0", port=5000, debug=False)
