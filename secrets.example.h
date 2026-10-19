#pragma once
// =====================================================================
// secrets.h : kredensial & alamat server.
// JANGAN di-commit ke GitHub / dibagikan di proyek Wokwi publik.
// Jika memakai Git, tambahkan "secrets.h" ke .gitignore dan simpan
// salinan kosongnya sebagai secrets.example.h.
// =====================================================================

// Kunci sederhana agar tidak sembarang orang bisa POST ke API kamu.
// Harus SAMA dengan WATER_API_KEY di server Python.
#define API_KEY "ganti-dengan-kunci-acak-milikmu"

// ---------------------------------------------------------------------
// Dipakai jika USE_WOKWI = 1
// Wokwi TIDAK bisa mengakses localhost komputermu (lihat penjelasan).
// Isi dengan salah satu:
//   a) URL publik dari tunnel, contoh: "https://abcd-1234.ngrok-free.app/api/water-level"
//   b) Private Gateway (berbayar): "http://host.wokwi.internal:8000/api/water-level"
// ---------------------------------------------------------------------
#define WOKWI_SERVER_URL "https://GANTI-DENGAN-URL-TUNNEL/api/water-level"

// ---------------------------------------------------------------------
// Dipakai jika USE_WOKWI = 0 (ESP32 fisik)
// WiFi harus 2.4 GHz. Gunakan IP LAN komputer server (bukan localhost).
// ---------------------------------------------------------------------
#define HOME_WIFI_SSID     "NAMA_WIFI_KAMU"
#define HOME_WIFI_PASSWORD "PASSWORD_WIFI_KAMU"
#define HOME_SERVER_URL    "http://192.168.1.10:8000/api/water-level"
