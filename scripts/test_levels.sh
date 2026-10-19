#!/usr/bin/env bash
# Tes otomatis: untuk tiap jarak, salin diagram.json dengan atribut "distance"
# diubah, jalankan wokwi-cli, lalu cek teks yang muncul di serial.
# Butuh: build/ sudah ada (scripts/build.sh) dan WOKWI_CLI_TOKEN terisi.
set -uo pipefail
cd "$(dirname "$0")/.."

command -v wokwi-cli >/dev/null || { echo "wokwi-cli belum terpasang"; exit 1; }
[ -f build/water_level_monitor.ino.bin ] || { echo "Jalankan scripts/build.sh dulu"; exit 1; }
: "${WOKWI_CLI_TOKEN:?Isi variabel WOKWI_CLI_TOKEN dulu}"
grep -Eq '"distance": *"[0-9.]+"' diagram.json || { echo 'diagram.json tidak punya atribut "distance" pada HC-SR04'; exit 1; }

# format: jarak_cm|teks_serial_yang_diharapkan
# (dengan CONTAINER_HEIGHT_CM=100, WARNING=40, DANGER=70 di config.h)
CASES=(
  "80|status=AMAN"
  "55|status=WASPADA"
  "25|status=BAHAYA"
  "400|SENSOR_ERROR"
)

fail=0
for c in "${CASES[@]}"; do
  dist="${c%%|*}"
  expect="${c#*|}"
  dir=".test/d${dist}"
  rm -rf "$dir"; mkdir -p "$dir"
  sed -E "s/\"distance\": *\"[0-9.]+\"/\"distance\": \"${dist}\"/" diagram.json > "$dir/diagram.json"
  sed -E "s#'build/#'../../build/#g" wokwi.toml > "$dir/wokwi.toml"

  echo "=== Jarak ${dist} cm | harapan di serial: '${expect}'"
  if wokwi-cli "$dir" --timeout 15000 --expect-text "$expect"; then
    echo ">>> LULUS"
  else
    echo ">>> GAGAL"
    fail=1
  fi
done

rm -rf .test
exit $fail
