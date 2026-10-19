#!/usr/bin/env bash
# Compile firmware ke folder build/ (dibaca oleh wokwi.toml).
# Nama folder proyek HARUS sama dengan nama file .ino (water_level_monitor).
set -euo pipefail
cd "$(dirname "$0")/.."

FQBN="esp32:esp32:esp32"

command -v arduino-cli >/dev/null || { echo "arduino-cli belum terpasang"; exit 1; }

arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "Adafruit BusIO"

arduino-cli compile --fqbn "$FQBN" --output-dir build .

echo
echo "Selesai. Cek: build/water_level_monitor.ino.bin dan build/water_level_monitor.ino.elf"
