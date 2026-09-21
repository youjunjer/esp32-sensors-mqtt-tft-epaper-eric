#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ESP32 Dev Module 腳位
const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

// Wi-Fi（沿用目前專案設定）
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT
const char *MQTT_HOST = "mqttgo.io";
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "eric/class305/data";
const unsigned long MQTT_INTERVAL = 10000UL;

const int TEMP_WARNING = 26;
const int TEMP_LIMIT = 28;
const int HUMIDITY_WARNING = 60;
const int HUMIDITY_LIMIT = 70;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int lastTemperature = 0;
int lastHumidity = 0;
int lastLight = 0;
unsigned long lastMqttTime = 0;

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 3, y + 1, 5, 10);
  oled.drawDisc(x + 5, y + 14, 3);
  oled.drawBox(x + 4, y + 6, 2, 9);
  oled.drawLine(x + 10, y + 4, x + 12, y + 4);
  oled.drawLine(x + 10, y + 8, x + 12, y + 8);
  oled.drawLine(x + 10, y + 12, x + 12, y + 12);
}

void drawDrop(int x, int y) {
  oled.drawTriangle(x + 6, y + 1, x, y + 8, x + 12, y + 8);
  oled.drawDisc(x + 6, y + 8, 6);
  oled.setDrawColor(0);
  oled.drawDisc(x + 4, y + 6, 1);
  oled.setDrawColor(1);
}

void drawSun(int x, int y) {
  oled.drawDisc(x + 6, y + 7, 3);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    int x1 = x + 6 + (int)(6 * cos(a));
    int y1 = y + 7 + (int)(6 * sin(a));
    int x2 = x + 6 + (int)(8 * cos(a));
    int y2 = y + 7 + (int)(8 * sin(a));
    oled.drawLine(x1, y1, x2, y2);
  }
}

void drawValue(int x, int baseline, int value, const char *unit) {
  oled.setFont(u8g2_font_10x20_tf);
  oled.setCursor(x, baseline);
  oled.print(value);
  oled.setFont(u8g2_font_5x8_tf);
  oled.print(unit);
}

void drawBottomValue(int x, int baseline, int value, const char *unit) {
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(x, baseline);
  oled.print(value);
  oled.setFont(u8g2_font_5x8_tf);
  oled.print(unit);
}

void showStatus(const char *line1, const char *line2, const char *line3) {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(8, 16); oled.print(line1);
  oled.setCursor(8, 34); oled.print(line2);
  oled.setCursor(8, 52); oled.print(line3);
  oled.sendBuffer();
}

void setLedState() {
  bool danger = lastTemperature > TEMP_LIMIT || lastHumidity > HUMIDITY_LIMIT;
  bool warning = lastTemperature >= TEMP_WARNING || lastHumidity >= HUMIDITY_WARNING;
  digitalWrite(GREEN_LED_PIN, !danger && !warning ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, !danger && warning ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, danger ? HIGH : LOW);
}

void showSensorData(bool dhtOK) {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.drawHLine(4, 32, 120);
  oled.drawVLine(64, 36, 24);
  drawThermometer(7, 6); drawDrop(8, 40); drawSun(72, 40);
  oled.setFont(u8g2_font_5x8_tf);
  oled.setCursor(24, 12); oled.print("TEMP");
  oled.setCursor(22, 44); oled.print("HUM");
  oled.setCursor(88, 44); oled.print("LIGHT");
  if (dhtOK) drawValue(45, 29, lastTemperature, " C");
  else { oled.setFont(u8g2_font_6x12_tf); oled.setCursor(45, 25); oled.print("ERROR"); }
  drawBottomValue(24, 61, lastHumidity, " %");
  drawBottomValue(91, 61, lastLight, " %");
  oled.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000UL) {
    showStatus("WiFi connection", "Please wait...", WIFI_SSID);
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    showStatus("WiFi connected", ip.c_str(), "MQTT starting...");
    delay(1000);
  } else showStatus("WiFi failed", "Check SSID/password", "Retry later");
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  while (!mqttClient.connected()) {
    String clientId = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    clientId += String(random(0xffff), HEX);
    showStatus("MQTT connection", "Connecting...", MQTT_HOST);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("MQTT connected");
      showStatus("MQTT connected", MQTT_TOPIC, "Ready");
      delay(800);
    } else {
      Serial.print("MQTT failed, state: "); Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

bool publishSensorData() {
  if (!mqttClient.connected()) return false;
  char payload[80];
  snprintf(payload, sizeof(payload), "{\"temp\":%d,\"humi\":%d,\"light\":%d}",
           lastTemperature, lastHumidity, lastLight);
  bool result = mqttClient.publish(MQTT_TOPIC, payload);
  Serial.print("MQTT payload: "); Serial.println(payload);
  return result;
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT); pinMode(YELLOW_LED_PIN, OUTPUT); pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW); digitalWrite(YELLOW_LED_PIN, LOW); digitalWrite(RED_LED_PIN, LOW);
  analogReadResolution(12);
  Wire.begin(SDA_PIN, SCL_PIN);
  oled.begin(); oled.setFontPosBaseline();
  randomSeed((uint32_t)ESP.getEfuseMac());
  showStatus("ESP32 SENSOR", "Starting...", "Please wait...");
  connectWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  byte temperature = 0, humidity = 0;
  int err = dht11.read(&temperature, &humidity, NULL);
  bool dhtOK = err == SimpleDHTErrSuccess;
  if (dhtOK) { lastTemperature = temperature; lastHumidity = humidity; }
  int lightRaw = analogRead(LIGHT_PIN);
  lastLight = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  setLedState();
  showSensorData(dhtOK);

  if (lastMqttTime == 0 || millis() - lastMqttTime >= MQTT_INTERVAL) {
    if (publishSensorData()) lastMqttTime = millis();
    showSensorData(dhtOK);
  }
  delay(1000);
}
