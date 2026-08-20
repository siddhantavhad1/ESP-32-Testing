#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// ---- CONFIG ----
const char* ap_ssid     = "ESP32-Counter";
const char* ap_password = "12345678";

const int IR_PIN  = 4;         // IR sensor OUT
const bool ACTIVE_LOW = true;  // set false if your IR sensor is active-high

#define DHT_PIN 15              // DHT11 DATA pin
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ---- STATE ----
volatile unsigned long objectCount = 0;
volatile unsigned long lastTriggerTime = 0;
const unsigned long DEBOUNCE_MS = 100;

float temperature = 0;
float humidity = 0;
unsigned long lastDHTRead = 0;
const unsigned long DHT_INTERVAL = 2000; // DHT11 is slow, read every 2s max

WebServer server(80);

void IRAM_ATTR handleIRInterrupt() {
  unsigned long now = millis();
  if (now - lastTriggerTime > DEBOUNCE_MS) {
    objectCount++;
    lastTriggerTime = now;
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<title>ESP32 Dashboard</title>";
  html += "<style>body{font-family:sans-serif;text-align:center;margin-top:40px;}";
  html += ".box{display:inline-block;margin:20px;padding:20px 40px;border-radius:12px;";
  html += "background:#ecf0f1;box-shadow:0 2px 6px rgba(0,0,0,0.15);}";
  html += "h1{font-size:60px;color:#2c3e50;margin:5px;}";
  html += "p{font-size:18px;color:#7f8c8d;margin:0;}</style></head><body>";
  html += "<h2>ESP32 Sensor Dashboard</h2>";

  html += "<div class='box'><p>OBJECT COUNT</p><h1>" + String(objectCount) + "</h1></div>";

  html += "<div class='box'><p>TEMPERATURE</p><h1>";
  html += isnan(temperature) ? "--" : String(temperature, 1);
  html += " &deg;C</h1></div>";

  html += "<div class='box'><p>HUMIDITY</p><h1>";
  html += isnan(humidity) ? "--" : String(humidity, 1);
  html += " %</h1></div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleReset() {
  objectCount = 0;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDataJson() {
  String json = "{";
  json += "\"count\":" + String(objectCount) + ",";
  json += "\"temperature\":" + String(isnan(temperature) ? -1 : temperature) + ",";
  json += "\"humidity\":" + String(isnan(humidity) ? -1 : humidity);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_PIN), handleIRInterrupt,
                   ACTIVE_LOW ? FALLING : RISING);

  dht.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println();
  Serial.print("Access Point started. SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Connect your PC to this WiFi, then browse to: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.on("/data", handleDataJson);
  server.begin();
}

void loop() {
  server.handleClient();

  // Read DHT11 periodically (non-blocking, doesn't interfere with IR interrupt)
  unsigned long now = millis();
  if (now - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;
  }
}