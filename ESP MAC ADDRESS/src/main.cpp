#include <Arduino.h>
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);

    Serial.print("esp32doit-devkit-v1 MAC: ");
    Serial.println(WiFi.macAddress());
    WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("STA MAC:  ");
  Serial.println(WiFi.macAddress());
  Serial.print("esp32doit-devkit-v1 MAC: ");
  Serial.print("AP MAC:   ");
  Serial.println(WiFi.softAPmacAddress());
}

void loop() {
}