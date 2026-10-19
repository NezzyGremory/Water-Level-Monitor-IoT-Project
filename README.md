# Water Level Monitoring System

Demo monitoring ketinggian air (risiko banjir): **ESP32 + HC-SR04 + OLED SSD1306 + 3 LED**,
data dikirim lewat **HTTP POST (JSON)** ke **API Python (FastAPI + SQLite)**.
Disimulasikan di **Wokwi**, dan bisa dikendalikan AI agent di VS Code lewat **Wokwi MCP**.

## Isi folder

```
water_level_monitor/            <- nama folder HARUS sama dengan nama .ino
├── water_level_monitor.ino     firmware utama
├── config.h                    pin, tinggi wadah, ambang status, interval, pilih Wokwi/fisik
├── secrets.h                   WiFi, URL server, API key   (di-.gitignore)
├── secrets.example.h           contoh kosong untuk dibagikan
├── diagram.json                rangkaian Wokwi (lihat catatan di bawah)
├── wokwi.toml                  menunjuk ke hasil build/
├── libraries.txt               untuk Wokwi di browser
├── AGENTS.md                   aturan untuk AI agent
├── .vscode/mcp.json            konfigurasi Wokwi MCP
├── .vscode/tasks.json          task Build / Tes / Server
├── scripts/build.sh | build.ps1   compile firmware
├── scripts/test_levels.sh      tes otomatis 4 skenario jarak
└── server/server.py, requirements.txt   API penerima data
```

## A. Opsi cepat: Wokwi di browser (tanpa VS Code)

1. Buka proyek Wokwi-mu yang sudah jalan.
2. Salin isi `water_level_monitor.ino` ke `sketch.ino`.
3. Tambah file `config.h` dan `secrets.h` (tombol "+" di editor), salin isinya.
4. Isi `libraries.txt` dengan isi file `libraries.txt`.
5. **Jangan timpa `diagram.json`-mu** yang sudah berfungsi.

## B. Opsi penuh: VS Code + AI agent

### 1. Pasang kebutuhan
- VS Code + agen yang mendukung MCP (Copilot Agent mode, Claude Code, Cursor, dll.)
- [arduino-cli](https://arduino.github.io/arduino-cli/)
- [wokwi-cli](https://github.com/wokwi/wokwi-cli):
  - Linux/macOS: `curl -L https://wokwi.com/ci/install.sh | sh`
  - Windows: `iwr https://wokwi.com/ci/install.ps1 -useb | iex`
- Python 3: `pip install -r server/requirements.txt`
- Token Wokwi dari Wokwi CI Dashboard, lalu set env var:
  `export WOKWI_CLI_TOKEN=...` (Windows: `setx WOKWI_CLI_TOKEN ...`)

### 2. Isi pengaturan
- `config.h`: sesuaikan `CONTAINER_HEIGHT_CM`, `WARNING_LEVEL_CM`, `DANGER_LEVEL_CM`.
- `secrets.h`: isi `API_KEY` dan `WOKWI_SERVER_URL` (lihat bagian Jaringan).

### 3. Build
```bash
bash scripts/build.sh          # Windows PowerShell: scripts\build.ps1
```
Hasil: `build/water_level_monitor.ino.bin` dan `.elf`.
Jika Wokwi gagal booting dari file `.bin`, coba ganti `firmware` di `wokwi.toml`
ke `build/water_level_monitor.ino.merged.bin` (dibuat juga oleh arduino-cli).

### 4. Jalankan simulasi
```bash
wokwi-cli .
```

### 5. Tes otomatis 4 skenario
```bash
bash scripts/test_levels.sh
```
Skrip mengubah atribut `distance` di salinan `diagram.json`, menjalankan simulasi, dan
mengecek teks serial. Butuh atribut `"distance"` pada HC-SR04 di `diagram.json`.

### 6. Hubungkan agen (Wokwi MCP)
1. Buka folder ini di VS Code (`.vscode/mcp.json` sudah ada).
2. Command Palette -> **MCP: List Servers** -> **Wokwi** -> **Start Server**, masukkan token.
3. Buka chat mode **Agent**, pastikan tool Wokwi muncul.
4. Contoh prompt:
   > Baca AGENTS.md. Build firmware, jalankan simulasi Wokwi, dan laporkan output serial
   > apa adanya. Jangan ubah pin.

Wokwi MCP berstatus eksperimental; nama tool/perilakunya bisa berubah.

### 7. Jalankan server API
```bash
cd server
export WATER_API_KEY="samakan-dengan-API_KEY-di-secrets.h"
uvicorn server:app --host 0.0.0.0 --port 8000
```
Dokumentasi interaktif: http://localhost:8000/docs
Data terbaru: `GET /api/water-level/latest?limit=10`

## Jaringan

| Lingkungan | `USE_WOKWI` | URL server |
|---|---|---|
| Wokwi | 1 | URL **publik** dari tunnel (Cloudflare Tunnel / ngrok), atau `http://host.wokwi.internal:8000/api/water-level` dengan Private Gateway (**berbayar**, Wokwi Club) |
| ESP32 fisik | 0 | `http://<IP-LAN-komputer>:8000/api/water-level` (WiFi 2.4 GHz, satu jaringan, firewall izinkan port 8000) |

`localhost` di ESP32/Wokwi bukan komputermu. Karena tunnel itu publik, API key wajib diganti
dan jangan dibagikan. HTTPS memakai `setInsecure()` (tanpa verifikasi sertifikat), hanya untuk demo.

## Perlindungan GPIO (ESP32 fisik)

- **ECHO HC-SR04 = 5 V, GPIO ESP32 maksimal 3,3 V.** Pasang pembagi tegangan:
  ECHO -> 1 kΩ -> GPIO 18, lalu GPIO 18 -> 2 kΩ -> GND (hasil ~3,3 V). Alternatif: level shifter
  atau HC-SR04P (versi 3,3 V).
- TRIG biasanya cukup dipicu 3,3 V, tapi tidak dijamin; uji di sensor-mu.
- Sensor ke 5 V (VIN), semua GND disatukan. OLED ke 3,3 V (alamat 0x3C atau 0x3D).
- LED wajib resistor seri 220–330 Ω.
- GPIO 5 adalah strapping pin; jangan ditahan rangkaian luar saat boot.
- HC-SR04 tidak tahan air. Untuk pemasangan nyata pertimbangkan sensor tahan air (mis. JSN-SR04T).

## Perilaku yang diharapkan (config bawaan: wadah 100 cm, waspada >= 40, bahaya >= 70)

| Jarak sensor | Tinggi air | Status | LED |
|---|---|---|---|
| 80 cm | 20 cm | AMAN | hijau |
| 55 cm | 45 cm | WASPADA | kuning |
| 25 cm | 75 cm | BAHAYA | merah |
| 400 cm / ECHO putus | - | SENSOR_ERROR | ketiganya berkedip |

## Contoh payload

```json
{"device_id":"wl-esp32-01","distance_cm":54.7,"water_level_cm":45.3,"status":"WASPADA","uptime_ms":12345}
{"device_id":"wl-esp32-01","distance_cm":null,"water_level_cm":null,"status":"SENSOR_ERROR","uptime_ms":20000}
```
Waktu penerimaan (`received_at`, UTC) ditambahkan server.

## Status verifikasi (jujur)

**Sudah dicek (di sandbox, bukan di perangkat):**
- `server.py`: POST valid, POST error (null), tanpa API key (401), status salah (422), dan GET latest.
- Sintaks firmware lewat stub library (`g++ -Wall -Wextra`), mode `USE_WOKWI` 1 dan 0.
- Logika sensor/status di PC dengan sensor tiruan: 80/55/25/400 cm, timeout, sampel kurang,
  lonjakan noise (median), dan toleransi dasar wadah.
- `diagram.json` valid sebagai JSON.

**Belum dicek:**
- Compile asli dengan core ESP32 dan library Adafruit.
- Simulasi di Wokwi (browser maupun CLI), termasuk tampilan OLED dan kedipan LED.
- Nama pin pada `diagram.json` buatan ini (`esp:5V`, `esp:GND.1`, dst.) dan tata letaknya.
  Bandingkan dengan `diagram.json`-mu yang sudah jalan atau hasil `wokwi-cli init`.
- Wokwi MCP, `scripts/test_levels.sh`, serta tunnel + HTTPS dari simulator.
- Seluruh perilaku di ESP32 fisik (pembagi tegangan, daya, stabilitas WiFi, alamat OLED).

## Keterbatasan yang diketahui
- Saat server lambat, satu siklus bisa tertahan hingga ~6 detik (timeout koneksi + respons).
- Belum ada hysteresis di ambang status (bisa berganti-ganti di sekitar batas).
- Server belum punya dashboard, notifikasi, atau autentikasi kuat.
