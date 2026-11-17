"""
Mengolah hasil pengujian menjadi tabel ringkasan dan grafik untuk laporan/presentasi.

Pemakaian:
    pip install matplotlib
    python analisis.py --akurasi data/akurasi.csv --serial data/serial_xxx.txt

Contoh dengan data contoh:
    python analisis.py --akurasi contoh/akurasi_contoh.csv --serial contoh/serial_contoh.txt

Hasil disimpan di folder hasil/: ringkasan.md, akurasi_per_skenario.png, waktu_inferensi.png
"""
import argparse
import csv
import os
import re
import statistics
from collections import OrderedDict

FOLDER = os.path.dirname(os.path.abspath(__file__))


def angka(teks):
    teks = (teks or "").strip()
    return int(teks) if teks else 0


def olah_akurasi(path):
    per_skenario = OrderedDict()
    with open(path, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            if not (r.get("masuk_alat") or "").strip():
                continue  # baris yang belum diisi
            s = per_skenario.setdefault(r["skenario"], {
                "ulangan": 0, "manual": 0, "alat": 0, "galat": 0,
                "terlewat": 0, "dobel": 0, "arah_terbalik": 0})
            mm, ma = angka(r["masuk_manual"]), angka(r["masuk_alat"])
            km, ka = angka(r["keluar_manual"]), angka(r["keluar_alat"])
            s["ulangan"] += 1
            s["manual"] += mm + km
            s["alat"] += ma + ka
            s["galat"] += abs(ma - mm) + abs(ka - km)
            for k in ("terlewat", "dobel", "arah_terbalik"):
                s[k] += angka(r[k])
    for s in per_skenario.values():
        s["akurasi"] = max(0.0, 1 - s["galat"] / s["manual"]) * 100 if s["manual"] else 0.0
    return per_skenario


def olah_serial(path):
    pola = re.compile(r"Inferensi:\s*(\d+)\s*ms")
    nilai = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for baris in f:
            m = pola.search(baris)
            if m:
                nilai.append(int(m.group(1)))
    return nilai


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--akurasi", help="CSV lembar akurasi")
    ap.add_argument("--serial", help="file log Serial Monitor")
    ap.add_argument("--keluaran", default=os.path.join(FOLDER, "hasil"))
    a = ap.parse_args()
    if not a.akurasi and not a.serial:
        ap.error("isi minimal --akurasi atau --serial")

    os.makedirs(a.keluaran, exist_ok=True)
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        plt = None
        print("matplotlib belum terpasang, grafik dilewati (pip install matplotlib).")

    md = ["# Ringkasan Hasil Pengujian", ""]

    if a.akurasi:
        data = olah_akurasi(a.akurasi)
        md += ["## Akurasi penghitungan", "",
               "| Skenario | Ulangan | Lintasan (manual) | Terhitung (alat) | Akurasi | Terlewat | Dobel | Arah terbalik |",
               "|---|---|---|---|---|---|---|---|"]
        total_manual = total_galat = 0
        for nama, s in data.items():
            md.append(f"| {nama} | {s['ulangan']} | {s['manual']} | {s['alat']} | {s['akurasi']:.1f}% "
                      f"| {s['terlewat']} | {s['dobel']} | {s['arah_terbalik']} |")
            total_manual += s["manual"]
            total_galat += s["galat"]
        if total_manual:
            md += ["", f"Akurasi keseluruhan: **{(1 - total_galat / total_manual) * 100:.1f}%** "
                       f"dari {total_manual} lintasan.", ""]
        if plt and data:
            fig, ax = plt.subplots(figsize=(8, 4.5))
            nama = list(data)
            ax.bar(nama, [data[n]["akurasi"] for n in nama], color="#1d3650")
            ax.set_ylim(0, 100)
            ax.set_ylabel("Akurasi (%)")
            ax.set_title("Akurasi penghitungan per skenario")
            for i, n in enumerate(nama):
                ax.text(i, data[n]["akurasi"] + 1.5, f"{data[n]['akurasi']:.0f}%", ha="center")
            plt.xticks(rotation=15, ha="right")
            fig.tight_layout()
            fig.savefig(os.path.join(a.keluaran, "akurasi_per_skenario.png"), dpi=150)
            md += ["![Akurasi per skenario](akurasi_per_skenario.png)", ""]

    if a.serial:
        nilai = olah_serial(a.serial)
        if nilai:
            rata = statistics.mean(nilai)
            sd = statistics.stdev(nilai) if len(nilai) > 1 else 0.0
            md += ["## Kinerja inferensi", "",
                   "| Ukuran | Nilai |", "|---|---|",
                   f"| Jumlah sampel | {len(nilai)} |",
                   f"| Rata-rata | {rata:.1f} ms |",
                   f"| Simpangan baku | {sd:.1f} ms |",
                   f"| Minimum / maksimum | {min(nilai)} / {max(nilai)} ms |",
                   f"| Perkiraan laju proses | {1000 / rata:.2f} frame per detik (inferensi saja) |", ""]
            if plt:
                fig, ax = plt.subplots(figsize=(8, 3.5))
                ax.plot(nilai, color="#1d3650", linewidth=1)
                ax.set_xlabel("Frame ke-")
                ax.set_ylabel("Waktu inferensi (ms)")
                ax.set_title("Waktu inferensi di ESP32-CAM")
                fig.tight_layout()
                fig.savefig(os.path.join(a.keluaran, "waktu_inferensi.png"), dpi=150)
                md += ["![Waktu inferensi](waktu_inferensi.png)", ""]
        else:
            md += ["## Kinerja inferensi", "", "Tidak ada baris `Inferensi: ... ms` di log.", ""]

    path = os.path.join(a.keluaran, "ringkasan.md")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(md))
    print("\n".join(md))
    print(f"\nTersimpan di {a.keluaran}")


if __name__ == "__main__":
    main()
