#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

// ---- ESP8266 (Display Node) STA MAC ----
uint8_t receiverMAC[] = {0x98, 0xF4, 0xAB, 0xF5, 0xE0, 0x00};

// ---- IR sensor ----
const int IR_PIN = 4;
const bool ACTIVE_LOW = true;  // set false if your IR sensor is active-high

// ---- DHT11 ----
#define DHT_PIN 15
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

volatile unsigned long objectCount = 0;
volatile unsigned long lastTriggerTime = 0;
const unsigned long DEBOUNCE_MS = 100;

typedef struct SensorData {
  unsigned long count;
  float temperature;
  float humidity;
} SensorData;

SensorData dataPacket;
esp_now_peer_info_t peerInfo;

void IRAM_ATTR handleIRInterrupt() {
  unsigned long now = millis();
  if (now - lastTriggerTime > DEBOUNCE_MS) {
    objectCount++;
    lastTriggerTime = now;
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_PIN), handleIRInterrupt,
                   ACTIVE_LOW ? FALLING : RISING);
  dht.begin();

  WiFi.mode(WIFI_STA);  // ESP-NOW requires STA mode, but does NOT connect to any router

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(onDataSent);

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sensor Node (ESP32) ready.");
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  dataPacket.count = objectCount;
  dataPacket.temperature = isnan(t) ? -1 : t;
  dataPacket.humidity = isnan(h) ? -1 : h;

  esp_now_send(receiverMAC, (uint8_t *)&dataPacket, sizeof(dataPacket));

  delay(2000);
}