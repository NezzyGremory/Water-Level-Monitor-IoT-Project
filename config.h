#pragma once
// =====================================================================
// config.h : semua pengaturan yang mungkin kamu ubah ada di file ini.
// =====================================================================
#include <Arduino.h>

// ---------------------------------------------------------------------
// 1. LINGKUNGAN
//    1 = simulator Wokwi, 0 = ESP32 fisik
// ---------------------------------------------------------------------
#define USE_WOKWI 1

// ---------------------------------------------------------------------
// 2. IDENTITAS PERANGKAT
// ---------------------------------------------------------------------
constexpr const char* DEVICE_ID = "wl-esp32-01";

// ---------------------------------------------------------------------
// 3. PIN (sama seperti rangkaian yang sudah berfungsi)
// ---------------------------------------------------------------------
constexpr uint8_t PIN_TRIG      = 5;
constexpr uint8_t PIN_ECHO      = 18;
constexpr uint8_t PIN_SDA       = 21;
constexpr uint8_t PIN_SCL       = 22;
constexpr uint8_t PIN_LED_GREEN  = 25;
constexpr uint8_t PIN_LED_YELLOW = 26;
constexpr uint8_t PIN_LED_RED    = 27;

// ---------------------------------------------------------------------
// 4. OLED SSD1306
// ---------------------------------------------------------------------
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS  = 0x3C;   // sebagian modul memakai 0x3D
constexpr int8_t  OLED_RESET    = -1;     // -1 = tanpa pin reset

// ---------------------------------------------------------------------
// 5. WADAH & AMBANG STATUS AIR  (UBAH SESUAI WADAH KAMU)
//
//    Sensor dipasang di atas wadah, menghadap ke bawah.
//    CONTAINER_HEIGHT_CM = jarak sensor ke dasar wadah saat KOSONG.
//    Tinggi air = CONTAINER_HEIGHT_CM - jarak terbaca.
// ---------------------------------------------------------------------
constexpr float CONTAINER_HEIGHT_CM = 100.0;  // jarak sensor -> dasar wadah
constexpr float WARNING_LEVEL_CM    = 40.0;   // tinggi air >= ini  -> WASPADA
constexpr float DANGER_LEVEL_CM     = 70.0;   // tinggi air >= ini  -> BAHAYA

// Toleransi: jika jarak terbaca sedikit melebihi dasar wadah (noise),
// tinggi air dianggap 0. Jika melebihi toleransi, dianggap ERROR.
constexpr float EMPTY_TOLERANCE_CM  = 3.0;

// ---------------------------------------------------------------------
// 6. PEMBACAAN SENSOR HC-SR04
// ---------------------------------------------------------------------
constexpr uint8_t       SAMPLE_COUNT      = 5;       // jumlah sampel per pembacaan
constexpr uint8_t       MIN_VALID_SAMPLES = 3;       // minimal sampel valid
constexpr uint16_t      SAMPLE_GAP_MS     = 60;      // jeda antar sampel
constexpr unsigned long ECHO_TIMEOUT_US   = 30000UL; // batas tunggu echo (~5 m)
constexpr float         SENSOR_MIN_CM     = 2.0;     // batas bawah HC-SR04
constexpr float         SENSOR_MAX_CM     = 400.0;   // batas atas HC-SR04
constexpr float         SOUND_CM_PER_US   = 0.0343;  // kecepatan suara (~20 C)

// ---------------------------------------------------------------------
// 7. INTERVAL WAKTU
// ---------------------------------------------------------------------
constexpr unsigned long READ_INTERVAL_MS  = 1000;   // baca sensor tiap 1 detik
constexpr unsigned long SEND_INTERVAL_MS  = 5000;   // kirim ke server tiap 5 detik
                                                    // (juga langsung kirim saat status berubah)
constexpr unsigned long WIFI_RETRY_MS     = 15000;  // coba sambung ulang WiFi
constexpr unsigned long HTTP_TIMEOUT_MS   = 3000;   // timeout koneksi & respons HTTP
constexpr unsigned long BLINK_INTERVAL_MS = 500;    // kedip LED saat sensor error

// ---------------------------------------------------------------------
// 8. JARINGAN  (kredensial dibaca dari secrets.h)
// ---------------------------------------------------------------------
#include "secrets.h"

#if USE_WOKWI
  constexpr const char* WIFI_SSID     = "Wokwi-GUEST";  // WiFi virtual Wokwi
  constexpr const char* WIFI_PASSWORD = "";             // tanpa password
  constexpr int         WIFI_CHANNEL  = 6;              // mempercepat koneksi
  constexpr const char* SERVER_URL    = WOKWI_SERVER_URL;
#else
  constexpr const char* WIFI_SSID     = HOME_WIFI_SSID;
  constexpr const char* WIFI_PASSWORD = HOME_WIFI_PASSWORD;
  constexpr int         WIFI_CHANNEL  = 0;              // 0 = otomatis
  constexpr const char* SERVER_URL    = HOME_SERVER_URL;
#endif

// ---------------------------------------------------------------------
// 9. TIPE DATA (diletakkan di sini agar aman dari auto-prototype Arduino)
// ---------------------------------------------------------------------
enum WaterStatus {
  STATUS_AMAN,
  STATUS_WASPADA,
  STATUS_BAHAYA,
  STATUS_SENSOR_ERROR
};

enum NetState {
  NET_UNKNOWN,   // belum pernah mencoba kirim
  NET_OK,        // pengiriman terakhir berhasil
  NET_FAIL       // pengiriman terakhir gagal
};
