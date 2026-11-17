"""
Merekam keluaran Serial ESP32-CAM ke file, untuk pengujian kinerja dan ketahanan.

Pemakaian:  pip install pyserial
            python catat_serial.py COM3            (Windows)
            python catat_serial.py /dev/ttyUSB0    (Linux)
Tutup Serial Monitor Arduino IDE terlebih dahulu. Hentikan dengan Ctrl+C.
Hasil disimpan ke data/serial_YYYY-MM-DD_HHMMSS.txt
"""
import os
import sys
from datetime import datetime

import serial

if len(sys.argv) < 2:
    sys.exit("Sebutkan port, contoh: python catat_serial.py COM3")

port = sys.argv[1]
folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
os.makedirs(folder, exist_ok=True)
path = os.path.join(folder, f"serial_{datetime.now():%Y-%m-%d_%H%M%S}.txt")

with serial.Serial(port, 115200, timeout=1) as ser, open(path, "w", encoding="utf-8") as f:
    print(f"Merekam {port} ke {path}. Tekan Ctrl+C untuk berhenti.")
    try:
        while True:
            baris = ser.readline().decode("utf-8", errors="replace").rstrip()
            if not baris:
                continue
            teks = f"{datetime.now():%H:%M:%S.%f}"[:-3] + " -> " + baris
            print(teks)
            f.write(teks + "\n")
            f.flush()
    except KeyboardInterrupt:
        print("\nSelesai.")
