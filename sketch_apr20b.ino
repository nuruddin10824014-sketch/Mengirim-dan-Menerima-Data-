// --- Praktikum ioT ---
// --- Muhamad Nuruddin Fahmi ---
// --- mengirim dan penerima data -- 
// --- Versi : 1 ---
// --- Pemilik : pribadi ---

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =========================
// WIFI & THINGSPEAK
// =========================
const char* ssid = "micro12";
const char* password = "micro123";
String apiKey = "HNPPKBS7E92Q9OJV";

// =========================
// PIN LORA
// =========================
#define LORA_NSS   8
#define LORA_SCK   9
#define LORA_MOSI 10
#define LORA_MISO 11
#define LORA_RST  12
#define LORA_BUSY 13
#define LORA_DIO1 14

// =========================
// PIN OLED
// =========================
#define OLED_SDA  17
#define OLED_SCL  18
#define OLED_RST  21

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
  U8G2_R0,
  OLED_SCL,
  OLED_SDA,
  OLED_RST
);

// =========================
// RADIO
// =========================
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // =========================
  // WIFI CONNECT
  // =========================
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  // OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "LoRa RX Starting...");
  u8g2.sendBuffer();

  // INIT LoRa
  int state = radio.begin(923.0, 125.0, 7, 5, 0x12, 14, 8);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init FAILED: ");
    Serial.println(state);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Radio ERROR!");
    u8g2.sendBuffer();

    while (true);
  }

  Serial.println("LoRa RX READY");

  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "LoRa RX READY");
  u8g2.sendBuffer();
}

void loop() {
  String str;

  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {

    float rssi = radio.getRSSI();
    float snr  = radio.getSNR();

    Serial.println("=== DATA MASUK ===");
    Serial.println(str);
    Serial.print("Data float: ");
    Serial.println(str.toFloat());
    // OLED
    String displayStr = str.substring(0, 20);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "DATA:");
    u8g2.drawStr(0, 28, displayStr.c_str());

    char info[32];
    snprintf(info, sizeof(info), "RSSI: %.1f", rssi);
    u8g2.drawStr(0, 44, info);

    snprintf(info, sizeof(info), "SNR : %.1f", snr);
    u8g2.drawStr(0, 60, info);

    u8g2.sendBuffer();

    // =========================
    // KIRIM KE THINGSPEAK
    // =========================
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String url = "http://api.thingspeak.com/update?api_key=" + apiKey +
             "&field1=" + String(str.toFloat()) +
             "&field2=" + String(rssi) +
             "&field3=" + String(snr);

      http.begin(url);
      int httpResponseCode = http.GET();

      Serial.print("HTTP Response: ");
      Serial.println(httpResponseCode);

      http.end();
    }

    // WAJIB delay (limit ThingSpeak)
    delay(15000);
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // normal
  }
  else {
    Serial.print("Receive ERROR: ");
    Serial.println(state);
  }
}