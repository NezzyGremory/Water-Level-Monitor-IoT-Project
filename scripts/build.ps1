# Versi PowerShell dari scripts/build.sh
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$wokwiBin = Join-Path $HOME ".wokwi\bin"
if (Test-Path $wokwiBin) {
    $env:PATH = "$wokwiBin;$env:PATH"
}

arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "Adafruit BusIO"
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build .

Write-Host "Selesai. Cek folder build/"
