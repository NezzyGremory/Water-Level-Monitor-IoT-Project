// =====================================================================
// Water Level Monitoring System - ESP32 + HC-SR04 + OLED SSD1306
// Indikator lokal (OLED + LED) selalu berjalan, walau WiFi/server mati.
// Pengaturan ada di config.h, kredensial ada di secrets.h.
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------------- Variabel status global -----------------------
bool displayOk = false;

WaterStatus currentStatus  = STATUS_SENSOR_ERROR;
WaterStatus lastSentStatus = STATUS_SENSOR_ERROR;
NetState    serverState    = NET_UNKNOWN;

bool  measurementValid = false;   // true hanya jika pengukuran sah
float lastDistanceCm   = 0.0;
float lastWaterLevelCm = 0.0;

unsigned long lastReadMs        = 0;
unsigned long lastSendMs        = 0;
unsigned long lastWifiAttemptMs = 0;
bool firstReadDone = false;
bool everSent      = false;

// ---------------------------- Teks status ----------------------------
// Dipakai untuk JSON (konsisten dengan server)
const char* statusToJson(WaterStatus s) {
  switch (s) {
    case STATUS_AMAN:    return "AMAN";
    case STATUS_WASPADA: return "WASPADA";
    case STATUS_BAHAYA:  return "BAHAYA";
    default:             return "SENSOR_ERROR";
  }
}

// Dipakai untuk OLED (lebih pendek agar muat)
const char* statusToDisplay(WaterStatus s) {
  switch (s) {
    case STATUS_AMAN:    return "AMAN";
    case STATUS_WASPADA: return "WASPADA";
    case STATUS_BAHAYA:  return "BAHAYA";
    default:             return "ERROR";
  }
}

const char* serverStateText() {
  switch (serverState) {
    case NET_OK:   return "OK";
    case NET_FAIL: return "ERR";
    default:       return "-";
  }
}

// ------------------------------- LED ---------------------------------
void setLeds(bool green, bool yellow, bool red) {
  digitalWrite(PIN_LED_GREEN,  green  ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_LED_RED,    red    ? HIGH : LOW);
}

// Dipanggil terus di loop() supaya kedipan saat ERROR berjalan mulus.
void updateLeds() {
  switch (currentStatus) {
    case STATUS_AMAN:    setLeds(true, false, false); break;
    case STATUS_WASPADA: setLeds(false, true, false); break;
    case STATUS_BAHAYA:  setLeds(false, false, true); break;
    default: {
      // Sensor error: ketiga LED berkedip bersamaan
      bool on = ((millis() / BLINK_INTERVAL_MS) % 2) == 0;
      setLeds(on, on, on);
    }
  }
}

void ledSelfTest() {
  setLeds(true, false, false);  delay(200);
  setLeds(false, true, false);  delay(200);
  setLeds(false, false, true);  delay(200);
  setLeds(false, false, false);
}

// ------------------------------ Sensor -------------------------------
// Satu kali pengukuran. Mengembalikan -1 jika timeout (tidak ada echo).
float readDistanceOnceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0;               // timeout
  return duration * SOUND_CM_PER_US / 2.0;      // pulang-pergi -> dibagi 2
}

// Ambil beberapa sampel, buang yang tidak valid, lalu pakai MEDIAN
// (lebih tahan terhadap lonjakan noise dibanding rata-rata).
// Mengembalikan false jika sampel valid kurang dari MIN_VALID_SAMPLES.
bool readDistanceFiltered(float &distanceCm) {
  float valid[SAMPLE_COUNT];
  int n = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float d = readDistanceOnceCm();
    if (d >= SENSOR_MIN_CM && d <= SENSOR_MAX_CM) {
      valid[n++] = d;
    }
    delay(SAMPLE_GAP_MS);
  }

  if (n < MIN_VALID_SAMPLES) return false;

  // Urutkan (insertion sort, cukup untuk array kecil)
  for (int i = 1; i < n; i++) {
    float key = valid[i];
    int j = i - 1;
    while (j >= 0 && valid[j] > key) {
      valid[j + 1] = valid[j];
      j--;
    }
    valid[j + 1] = key;
  }

  if (n % 2 == 1) distanceCm = valid[n / 2];
  else            distanceCm = (valid[n / 2 - 1] + valid[n / 2]) / 2.0;
  return true;
}

// Hitung tinggi air & tentukan status dari jarak yang sudah valid.
WaterStatus evaluateStatus(float distanceCm, float &levelCm) {
  // Jarak jauh melebihi dasar wadah -> pembacaan tidak masuk akal
  if (distanceCm > CONTAINER_HEIGHT_CM + EMPTY_TOLERANCE_CM) {
    return STATUS_SENSOR_ERROR;
  }
  levelCm = CONTAINER_HEIGHT_CM - distanceCm;
  if (levelCm < 0) levelCm = 0;   // selisih kecil karena noise

  if (levelCm >= DANGER_LEVEL_CM)  return STATUS_BAHAYA;
  if (levelCm >= WARNING_LEVEL_CM) return STATUS_WASPADA;
  return STATUS_AMAN;
}

// Satu siklus: baca sensor -> hitung -> simpan ke variabel global.
void measure() {
  float distance = 0.0;
  float level = 0.0;

  if (!readDistanceFiltered(distance)) {
    measurementValid = false;
    currentStatus = STATUS_SENSOR_ERROR;
    Serial.println("[SENSOR] Tidak ada echo valid (timeout/di luar jangkauan)");
    return;
  }

  WaterStatus s = evaluateStatus(distance, level);
  if (s == STATUS_SENSOR_ERROR) {
    measurementValid = false;
    currentStatus = STATUS_SENSOR_ERROR;
    Serial.printf("[SENSOR] Jarak %.1f cm melebihi tinggi wadah, dianggap tidak valid\n", distance);
    return;
  }

  measurementValid = true;
  lastDistanceCm   = distance;
  lastWaterLevelCm = level;
  currentStatus    = s;
  Serial.printf("[SENSOR] jarak=%.1f cm | tinggi air=%.1f cm | status=%s\n",
                distance, level, statusToJson(s));
}

// ------------------------------- OLED --------------------------------
void drawDisplay() {
  if (!displayOk) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Water Level Monitor");

  display.setTextSize(2);
  display.setCursor(0, 11);
  if (measurementValid) {
    display.print(lastWaterLevelCm, 1);
    display.print(" cm");
  } else {
    display.print("-- cm");
  }

  display.setTextSize(1);
  display.setCursor(0, 30);
  if (measurementValid) {
    display.print("Sensor: ");
    display.print(lastDistanceCm, 1);
    display.print(" cm");
  } else {
    display.print("Sensor: tidak valid");
  }

  display.setTextSize(2);
  display.setCursor(0, 40);
  display.print(statusToDisplay(currentStatus));

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("WiFi:");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "--");
  display.print(" Srv:");
  display.print(serverStateText());

  display.display();
}

// ------------------------------- WiFi --------------------------------
// Tidak memblokir: WiFi disambungkan di latar belakang.
void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  lastWifiAttemptMs = millis();
  Serial.printf("[WIFI] Menyambung ke %s ...\n", WIFI_SSID);
}

void maintainWifi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) return;
  if (now - lastWifiAttemptMs >= WIFI_RETRY_MS) {
    Serial.println("[WIFI] Belum tersambung, mencoba lagi...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    lastWifiAttemptMs = now;
  }
}

// ----------------------------- HTTP POST -----------------------------
// Membuat JSON. Jika pengukuran tidak valid, nilai dikirim sebagai null
// supaya server tidak menganggapnya data normal.
void buildPayload(char* buf, size_t len) {
  if (measurementValid) {
    snprintf(buf, len,
      "{\"device_id\":\"%s\",\"distance_cm\":%.1f,\"water_level_cm\":%.1f,"
      "\"status\":\"%s\",\"uptime_ms\":%lu}",
      DEVICE_ID, lastDistanceCm, lastWaterLevelCm,
      statusToJson(currentStatus), millis());
  } else {
    snprintf(buf, len,
      "{\"device_id\":\"%s\",\"distance_cm\":null,\"water_level_cm\":null,"
      "\"status\":\"%s\",\"uptime_ms\":%lu}",
      DEVICE_ID, statusToJson(currentStatus), millis());
  }
}

bool sendToServer(const char* payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Lewati: WiFi belum tersambung");
    return false;
  }

  WiFiClient       plainClient;
  WiFiClientSecure secureClient;
  HTTPClient       http;

  bool useHttps = String(SERVER_URL).startsWith("https://");
  bool begun;
  if (useHttps) {
    secureClient.setInsecure();   // DEMO saja: sertifikat server tidak diverifikasi
    begun = http.begin(secureClient, SERVER_URL);
  } else {
    begun = http.begin(plainClient, SERVER_URL);
  }
  if (!begun) {
    Serial.println("[HTTP] URL tidak valid / gagal memulai koneksi");
    return false;
  }

  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int code = http.POST(String(payload));
  http.end();

  if (code >= 200 && code < 300) {
    Serial.printf("[HTTP] Terkirim, kode %d\n", code);
    return true;
  }
  // code negatif = error koneksi/timeout, positif = ditolak server
  Serial.printf("[HTTP] Gagal, kode %d\n", code);
  return false;
}

bool shouldSend(unsigned long now) {
  if (!everSent) return true;                          // kirim pertama
  if (currentStatus != lastSentStatus) return true;    // status berubah: kirim segera
  return (now - lastSendMs) >= SEND_INTERVAL_MS;       // kirim berkala
}

void handleSending() {
  char payload[200];
  buildPayload(payload, sizeof(payload));
  Serial.printf("[HTTP] Payload: %s\n", payload);

  bool ok = sendToServer(payload);

  // Dicatat walau gagal, supaya tidak mencoba terus-menerus tiap detik
  lastSendMs     = millis();
  lastSentStatus = currentStatus;
  everSent       = true;
  serverState    = ok ? NET_OK : NET_FAIL;
}

// ----------------------------- setup/loop ----------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Water Level Monitoring System ===");

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);
  ledSelfTest();

  Wire.begin(PIN_SDA, PIN_SCL);
  displayOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (displayOk) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Water Level Monitor");
    display.println();
    display.println("Memulai...");
    display.display();
  } else {
    Serial.println("[OLED] Tidak terdeteksi, lanjut tanpa OLED");
  }

  startWifi();
}

void loop() {
  unsigned long now = millis();

  maintainWifi(now);

  if (!firstReadDone || (now - lastReadMs) >= READ_INTERVAL_MS) {
    firstReadDone = true;
    lastReadMs = now;

    // 1) Indikator lokal selalu diperbarui lebih dulu
    measure();
    updateLeds();
    drawDisplay();

    // 2) Baru kirim ke server (bisa memakan waktu bila jaringan lambat)
    if (shouldSend(now)) {
      handleSending();
      drawDisplay();   // perbarui status "Srv:" di OLED
    }
  }

  updateLeds();   // menjaga kedipan LED saat sensor error
  delay(10);
}
