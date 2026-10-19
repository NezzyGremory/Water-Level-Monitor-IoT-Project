# Instruksi untuk AI agent (Water Level Monitoring System)

Proyek: monitoring ketinggian air (demo risiko banjir) dengan ESP32 + HC-SR04 +
OLED SSD1306 + 3 LED, mengirim JSON via HTTP POST ke API Python (FastAPI).
Pengguna adalah mahasiswa Sains Data pemula: utamakan kode sederhana & mudah dibaca.

## Aturan wajib
1. Jangan mengubah pin atau merombak rangkaian tanpa alasan teknis yang jelas.
   Pin: TRIG=5, ECHO=18, SDA=21, SCL=22, LED hijau=25, kuning=26, merah=27.
2. Pengaturan (tinggi wadah, ambang, interval, pin) hanya di `config.h`.
   Kredensial & URL hanya di `secrets.h` (jangan di-commit, jangan ditampilkan di jawaban).
3. OLED dan LED harus tetap bekerja walau WiFi/server mati. Jangan blokir loop lama.
4. Pengukuran tidak valid TIDAK boleh tampil/terkirim sebagai data normal
   (status `SENSOR_ERROR`, nilai `null`).
5. Kontrak JSON: `device_id`, `distance_cm`, `water_level_cm`, `status`, `uptime_ms`.
   Status: AMAN | WASPADA | BAHAYA | SENSOR_ERROR. Ubah firmware dan `server/server.py` bersamaan.
6. Tanpa Machine Learning, tanpa layanan cloud berbayar.
7. JANGAN mengarang hasil pengujian. Laporkan hanya output serial/log yang benar-benar
   keluar dari perintah yang kamu jalankan. Bedakan "diuji di simulator" dan "diuji di hardware fisik".
   Jika tidak bisa menjalankan sesuatu, katakan terus terang.
8. `localhost` di Wokwi bukan komputer pengguna. Pakai URL tunnel atau Private Gateway
   (`host.wokwi.internal`, berbayar). Untuk ESP32 fisik pakai IP LAN.

## Perintah
- Build      : `bash scripts/build.sh`  (hasil di `build/`)
- Simulasi   : `wokwi-cli .`  (butuh WOKWI_CLI_TOKEN)
- Tes level  : `bash scripts/test_levels.sh`
- Server API : `cd server && uvicorn server:app --host 0.0.0.0 --port 8000`
- Screenshot OLED: `wokwi-cli . --timeout 5000 --screenshot-part oled --screenshot-time 4500 --screenshot-file oled.png`

## Cara verifikasi
Setelah mengubah firmware: build -> jalankan simulasi -> baca serial -> cocokkan dengan
perilaku yang diharapkan (jarak 80/55/25/400 cm -> AMAN/WASPADA/BAHAYA/ERROR dengan config bawaan).
