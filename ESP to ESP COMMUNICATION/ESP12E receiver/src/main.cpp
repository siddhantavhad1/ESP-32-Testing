#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP8266WebServer.h>

const char* ap_ssid     = "ESP32-Dashboard";
const char* ap_password = "12345678";

typedef struct SensorData {
  unsigned long count;
  float temperature;
  float humidity;
} SensorData;

SensorData receivedData = {0, -1, -1};
unsigned long lastReceived = 0;

ESP8266WebServer server(80);

void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  lastReceived = millis();
}

void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP-NOW Sensor Dashboard</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body {
    background: #0a0e14;
    color: #d0f0ff;
    font-family: 'Courier New', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 30px 15px;
  }
  h1 {
    color: #00ffc8;
    font-size: 22px;
    letter-spacing: 2px;
    text-transform: uppercase;
    margin-bottom: 5px;
    text-shadow: 0 0 8px #00ffc8aa;
  }
  .status {
    font-size: 12px;
    color: #666;
    margin-bottom: 30px;
    letter-spacing: 1px;
  }
  .status.online { color: #00ff88; }
  .status.offline { color: #ff4444; }
  .dot {
    display:inline-block; width:8px; height:8px; border-radius:50%;
    margin-right:6px; background:#00ff88; box-shadow:0 0 6px #00ff88;
    animation: pulse 1.5s infinite;
  }
  .status.offline .dot { background:#ff4444; box-shadow:0 0 6px #ff4444; animation:none; }
  @keyframes pulse { 0%,100%{opacity:1;} 50%{opacity:0.3;} }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 20px;
    width: 100%;
    max-width: 800px;
  }
  .card {
    background: linear-gradient(145deg, #111820, #0d1218);
    border: 1px solid #1e2a35;
    border-radius: 10px;
    padding: 25px 20px;
    text-align: center;
    position: relative;
    overflow: hidden;
    transition: border-color 0.3s;
  }
  .card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
    background: linear-gradient(90deg, transparent, #00ffc8, transparent);
  }
  .card:hover { border-color: #00ffc855; }
  .label {
    font-size: 11px;
    color: #5a7a8a;
    letter-spacing: 2px;
    text-transform: uppercase;
    margin-bottom: 12px;
  }
  .value {
    font-size: 42px;
    font-weight: bold;
    color: #00ffc8;
    text-shadow: 0 0 12px #00ffc855;
  }
  .unit {
    font-size: 16px;
    color: #4a6a7a;
    margin-left: 4px;
  }
  .value.temp { color: #ff9d4d; text-shadow: 0 0 12px #ff9d4d55; }
  .value.hum { color: #4dc8ff; text-shadow: 0 0 12px #4dc8ff55; }

  .footer {
    margin-top: 30px;
    font-size: 11px;
    color: #445;
    letter-spacing: 1px;
  }
</style>
</head>
<body>
  <h1>&#9679; Sensor Dashboard</h1>
  <div class="status online" id="statusLine"><span class="dot"></span><span id="statusText">LINK ACTIVE</span></div>

  <div class="grid">
    <div class="card">
      <div class="label">Object Count</div>
      <div class="value" id="count">--</div>
    </div>
    <div class="card">
      <div class="label">Temperature</div>
      <div class="value temp"><span id="temp">--</span><span class="unit">&deg;C</span></div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="value hum"><span id="hum">--</span><span class="unit">%</span></div>
    </div>
  </div>

  <div class="footer">ESP-NOW &bull; ESP32 &#8594; ESP8266 &bull; AUTO-REFRESH 1s</div>

<script>
async function poll() {
  try {
    const res = await fetch('/data');
    const d = await res.json();
    document.getElementById('count').textContent = d.count;
    document.getElementById('temp').textContent = d.temperature >= 0 ? d.temperature.toFixed(1) : '--';
    document.getElementById('hum').textContent = d.humidity >= 0 ? d.humidity.toFixed(1) : '--';

    const statusLine = document.getElementById('statusLine');
    const statusText = document.getElementById('statusText');
    if (d.age_ms > 10000) {
      statusLine.className = 'status offline';
      statusText.textContent = 'NO SIGNAL (' + Math.round(d.age_ms/1000) + 's ago)';
    } else {
      statusLine.className = 'status online';
      statusText.textContent = 'LINK ACTIVE';
    }
  } catch (e) {
    document.getElementById('statusText').textContent = 'CONNECTION ERROR';
  }
}
poll();
setInterval(poll, 1000);
</script>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

void handleDataJson() {
  String json = "{";
  json += "\"count\":" + String(receivedData.count) + ",";
  json += "\"temperature\":" + String(receivedData.temperature) + ",";
  json += "\"humidity\":" + String(receivedData.humidity) + ",";
  json += "\"age_ms\":" + String(millis() - lastReceived);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("Display Node AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  server.on("/", handleRoot);
  server.on("/data", handleDataJson);
  server.begin();
}

void loop() {
  server.handleClient();
}