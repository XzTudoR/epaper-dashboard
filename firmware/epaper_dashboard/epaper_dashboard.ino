/**
 * ==============================================================================
 * Project: ESP32 E-Paper Smart Dashboard
 * Display: Good Display GDEY042T81 (4.2" 400x300 B/W E-Paper)
 * Backend: Google Apps Script (Calendar, Tasks, Open-Meteo Weather)
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// Configuration & Secrets
#if __has_include("config.h")
  #include "config.h"
#else
  #warning "config.h not found! Using template values from config.h.example. Please create config.h with your real credentials."
  #include "config.h.example"
#endif

#include "icons.h"
#include "display_ui.h"

// Hardware instances
DisplayUI::DashboardDisplay display(GxEPD2_420_GDEY042T81(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Forward declarations
bool connectWiFi();
bool fetchDashboardDataAndRender();
void goToDeepSleep();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[SYSTEM] ESP32 E-Paper Dashboard starting...");

  // Initialize Hardware SPI for E-Paper Display
  SPI.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, PIN_EPD_CS);
  display.init(115200, true, 2, false);
  display.setRotation(0);

  // Initialize U8G2 Font Engine
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  // Connect to WiFi and fetch updates
  if (connectWiFi()) {
    // Configure NTP Timezone
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
    setenv("TZ", NTP_TIMEZONE, 1);
    tzset();

    // Fetch backend data and update e-paper screen
    fetchDashboardDataAndRender();
  } else {
    Serial.println("[ERROR] WiFi connection failed. Skipping display update.");
  }

  // Put display and ESP32 into low-power deep sleep
  goToDeepSleep();
}

void loop() {
  // Empty - system reboots on wakeup from deep sleep
}

/**
 * Connect to WiFi network with timeout retries
 */
bool connectWiFi() {
  Serial.printf("[WiFi] Connecting to SSID: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  constexpr int MAX_ATTEMPTS = 20;

  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP Address: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }

  Serial.println("\n[WiFi] Connection timed out.");
  return false;
}

/**
 * Fetch JSON payload from Google Apps Script endpoint and render UI
 */
bool fetchDashboardDataAndRender() {
  Serial.println("[HTTP] Fetching dashboard data from Google Apps Script...");

  WiFiClientSecure client;
  client.setInsecure(); // Google Apps Script redirects across google domains

  HTTPClient http;
  http.begin(client, API_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000); // 15 second network timeout

  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  Serial.printf("[HTTP] Response code: %d\n", httpCode);
  if (httpCode != HTTP_CODE_OK && httpCode != 200) {
    Serial.printf("[HTTP] Unexpected response status: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError jsonError = deserializeJson(doc, payload);
  if (jsonError) {
    Serial.printf("[JSON] Deserialization failed: %s\n", jsonError.c_str());
    return false;
  }

  Serial.println("[DISPLAY] Rendering dashboard to E-Paper...");
  DisplayUI::renderDashboard(display, u8g2Fonts, doc);
  Serial.println("[DISPLAY] Screen refresh complete.");

  return true;
}

/**
 * Power down peripherals and enter ultra-low power deep sleep
 */
void goToDeepSleep() {
  Serial.println("[POWER] Hibernating display...");
  display.hibernate();

  Serial.println("[POWER] Disconnecting WiFi radio...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.printf("[POWER] Entering deep sleep for %llu seconds...\n", TIME_TO_SLEEP_SEC);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SEC * US_TO_S_FACTOR);
  esp_deep_sleep_start();
}
