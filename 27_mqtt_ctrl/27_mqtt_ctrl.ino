#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 感測器與 OLED 腳位
const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// LED：電燈、風扇、除濕機
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *MQTT_HOST = "mqttgo.io";
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC_GREEN = "eric/class305/ctrl/gled";
const char *MQTT_TOPIC_YELLOW = "eric/class305/ctrl/yled";
const char *MQTT_TOPIC_RED = "eric/class305/ctrl/rled";

const unsigned long SENSOR_INTERVAL = 10000UL;
const unsigned long COMMAND_DISPLAY_TIME = 1000UL;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int temperature = 0;
int humidity = 0;
int light = 0;
unsigned long lastSensorTime = 0;
unsigned long commandDisplayUntil = 0;
String lastCommand = "";

void showStatus(const char *a, const char *b, const char *c) {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(8, 16); oled.print(a);
  oled.setCursor(8, 34); oled.print(b);
  oled.setCursor(8, 52); oled.print(c);
  oled.sendBuffer();
}

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
  oled.setDrawColor(0); oled.drawDisc(x + 4, y + 6, 1); oled.setDrawColor(1);
}

void drawSun(int x, int y) {
  oled.drawDisc(x + 6, y + 7, 3);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    oled.drawLine(x + 6 + (int)(6 * cos(a)), y + 7 + (int)(6 * sin(a)),
                  x + 6 + (int)(8 * cos(a)), y + 7 + (int)(8 * sin(a)));
  }
}

void drawBottomValue(int x, int baseline, int value, const char *unit) {
  oled.setFont(u8g2_font_6x12_tf); oled.setCursor(x, baseline); oled.print(value);
  oled.setFont(u8g2_font_5x8_tf); oled.print(unit);
}

void showSensorData(bool dhtOK) {
  oled.clearBuffer(); oled.drawRFrame(0, 0, 128, 64, 4);
  oled.drawHLine(4, 32, 120); oled.drawVLine(64, 36, 24);
  drawThermometer(7, 6); drawDrop(8, 40); drawSun(72, 40);
  oled.setFont(u8g2_font_5x8_tf);
  oled.setCursor(24, 12); oled.print("TEMP");
  oled.setCursor(22, 44); oled.print("HUM");
  oled.setCursor(88, 44); oled.print("LIGHT");
  if (dhtOK) {
    oled.setFont(u8g2_font_10x20_tf); oled.setCursor(45, 29); oled.print(temperature);
    oled.setFont(u8g2_font_5x8_tf); oled.print(" C");
  } else {
    oled.setFont(u8g2_font_6x12_tf); oled.setCursor(45, 25); oled.print("ERROR");
  }
  drawBottomValue(24, 61, humidity, " %");
  drawBottomValue(91, 61, light, " %");
  oled.sendBuffer();
}

void showCommand(const String &command) {
  oled.clearBuffer(); oled.drawRFrame(0, 0, 128, 64, 4);
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(8, 16); oled.print("MQTT COMMAND");
  oled.setCursor(8, 36); oled.print(command);
  oled.setCursor(8, 54); oled.print("Command received");
  oled.sendBuffer();
}

bool hasJsonState(const String &json, const char *value) {
  String pattern = String("\"state\":\"") + value + "\"";
  return json.indexOf(pattern) >= 0;
}

void handleLedCommand(const String &json, int pin, const char *name) {
  if (hasJsonState(json, "on")) {
    digitalWrite(pin, HIGH);
    lastCommand = String(name) + " ON";
  } else if (hasJsonState(json, "off")) {
    digitalWrite(pin, LOW);
    lastCommand = String(name) + " OFF";
  } else return;
  showCommand(lastCommand);
  commandDisplayUntil = millis() + COMMAND_DISPLAY_TIME;
  Serial.print("MQTT command: "); Serial.println(lastCommand);
}

// MQTT callback：收到訊息後立即處理，不等待下一個感測週期
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String json;
  json.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) json += (char)payload[i];
  Serial.print("Received ["); Serial.print(topic); Serial.print("]: "); Serial.println(json);
  String receivedTopic = String(topic);
  if (receivedTopic == MQTT_TOPIC_GREEN) handleLedCommand(json, GREEN_LED_PIN, "LIGHT");
  else if (receivedTopic == MQTT_TOPIC_YELLOW) handleLedCommand(json, YELLOW_LED_PIN, "FAN");
  else if (receivedTopic == MQTT_TOPIC_RED) handleLedCommand(json, RED_LED_PIN, "DEHUM");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000UL) {
    showStatus("WiFi connection", "Please wait...", WIFI_SSID); delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    showStatus("WiFi connected", ip.c_str(), "MQTT starting..."); delay(800);
  } else showStatus("WiFi failed", "Check SSID/password", "Retry later");
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return;
  while (!mqttClient.connected()) {
    String clientId = "esp32-ctrl-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    clientId += String(random(0xffff), HEX);
    showStatus("MQTT connection", "Connecting...", MQTT_HOST);
    if (mqttClient.connect(clientId.c_str())) {
      mqttClient.subscribe(MQTT_TOPIC_GREEN);
      mqttClient.subscribe(MQTT_TOPIC_YELLOW);
      mqttClient.subscribe(MQTT_TOPIC_RED);
      Serial.println("Subscribed to green/yellow/red control topics");
      showStatus("MQTT subscribed", "3 control topics", "Waiting command"); delay(800);
    } else {
      Serial.print("MQTT failed, state: "); Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

void readSensors() {
  byte dhtTemp = 0, dhtHumidity = 0;
  int err = dht11.read(&dhtTemp, &dhtHumidity, NULL);
  if (err == SimpleDHTErrSuccess) { temperature = dhtTemp; humidity = dhtHumidity; }
  light = constrain(map(analogRead(LIGHT_PIN), 0, 4095, 0, 100), 0, 100);
  if (millis() >= commandDisplayUntil) showSensorData(err == SimpleDHTErrSuccess);
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT); pinMode(YELLOW_LED_PIN, OUTPUT); pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW); digitalWrite(YELLOW_LED_PIN, LOW); digitalWrite(RED_LED_PIN, LOW);
  analogReadResolution(12); Wire.begin(SDA_PIN, SCL_PIN);
  oled.begin(); oled.setFontPosBaseline(); randomSeed((uint32_t)ESP.getEfuseMac());
  showStatus("ESP32 CTRL", "Starting...", "Please wait...");
  connectWiFi(); mqttClient.setServer(MQTT_HOST, MQTT_PORT); mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();
  if (millis() - lastSensorTime >= SENSOR_INTERVAL) { lastSensorTime = millis(); readSensors(); }
  delay(10);
}
