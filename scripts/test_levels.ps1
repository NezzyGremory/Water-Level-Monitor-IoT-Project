# Versi PowerShell dari scripts/test_levels.sh
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$wokwiBin = Join-Path $HOME ".wokwi\bin"
if (Test-Path $wokwiBin) {
    $env:PATH = "$wokwiBin;$env:PATH"
}

if (-not (Get-Command "wokwi-cli" -ErrorAction SilentlyContinue)) {
    Write-Error "wokwi-cli belum terpasang atau belum ada di PATH."
    exit 1
}

if (-not (Test-Path "build\water_level_monitor.ino.bin")) {
    Write-Error "File build belum ada. Jalankan .\scripts\build.ps1 terlebih dahulu."
    exit 1
}

$token = [System.Environment]::GetEnvironmentVariable("WOKWI_CLI_TOKEN")
if (-not $token) {
    $token = [System.Environment]::GetEnvironmentVariable("WOKWI_CLI_TOKEN", "User")
}
if (-not $token) {
    Write-Error "Variabel WOKWI_CLI_TOKEN belum disetel."
    exit 1
}
$env:WOKWI_CLI_TOKEN = $token

$cases = @(
    @{ Distance = "80";  Expect = "status=AMAN" },
    @{ Distance = "55";  Expect = "status=WASPADA" },
    @{ Distance = "25";  Expect = "status=BAHAYA" },
    @{ Distance = "400"; Expect = "SENSOR_ERROR" }
)

$allPassed = $true

foreach ($test in $cases) {
    $dist = $test.Distance
    $expect = $test.Expect
    $dir = ".test\d$dist"

    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    New-Item -ItemType Directory -Path $dir -Force | Out-Null

    # Modifikasi distance di diagram.json
    $diagramContent = Get-Content "diagram.json" -Raw
    $newDiagram = $diagramContent -replace '"distance":\s*"[0-9.]+"', "`"distance`": `"$dist`""
    Set-Content -Path "$dir\diagram.json" -Value $newDiagram

    # Modifikasi path firmware di wokwi.toml
    $tomlContent = Get-Content "wokwi.toml" -Raw
    $newToml = $tomlContent -replace "'build/", "'../../build/"
    Set-Content -Path "$dir\wokwi.toml" -Value $newToml

    Write-Host "`n=== Uji Jarak $dist cm | Harapan serial: '$expect' ===" -ForegroundColor Cyan
    $output = wokwi-cli "$dir" --timeout 15000 --expect-text "$expect" 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host ">>> LULUS" -ForegroundColor Green
    } else {
        Write-Host ">>> GAGAL" -ForegroundColor Red
        Write-Host $output
        $allPassed = $false
    }
}

if (Test-Path ".test") {
    Remove-Item -Recurse -Force ".test"
}

if ($allPassed) {
    Write-Host "`nSemua skenario pengujian berhasil lulus!" -ForegroundColor Green
} else {
    Write-Host "`nAda skenario pengujian yang gagal." -ForegroundColor Red
    exit 1
}

