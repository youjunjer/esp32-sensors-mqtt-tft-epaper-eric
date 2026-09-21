#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ESP32 Dev Module 腳位
const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Wi-Fi 與 ThingSpeak 設定
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *THINGSPEAK_KEY = "YOUR_THINGSPEAK_WRITE_KEY";
const unsigned long UPLOAD_INTERVAL = 15000UL;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

int lastTemperature = 0;
int lastHumidity = 0;
int lastLight = 0;
int lastUploadCode = 0;
unsigned long lastUploadTime = 0;

void drawThermometer(int x, int y) {
  oled.drawFrame(x + 4, y + 1, 6, 12);
  oled.drawDisc(x + 7, y + 15, 4);
  oled.drawBox(x + 6, y + 6, 3, 10);
  oled.drawLine(x + 12, y + 4, x + 14, y + 4);
  oled.drawLine(x + 12, y + 8, x + 14, y + 8);
  oled.drawLine(x + 12, y + 12, x + 14, y + 12);
}

void drawDrop(int x, int y) {
  oled.drawTriangle(x + 7, y + 1, x, y + 10, x + 14, y + 10);
  oled.drawDisc(x + 7, y + 10, 7);
  oled.setDrawColor(0);
  oled.drawDisc(x + 5, y + 8, 2);
  oled.setDrawColor(1);
}

void drawSun(int x, int y) {
  oled.drawDisc(x + 7, y + 8, 4);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    int x1 = x + 7 + (int)(7 * cos(a));
    int y1 = y + 8 + (int)(7 * sin(a));
    int x2 = x + 7 + (int)(10 * cos(a));
    int y2 = y + 8 + (int)(10 * sin(a));
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

void showWiFiStatus(const char *line1, const char *line2, const char *line3) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(8, 16);
  oled.print(line1);
  oled.setCursor(8, 34);
  oled.print(line2);
  oled.setCursor(8, 52);
  oled.print(line3);
  oled.sendBuffer();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  int dots = 0;
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000UL) {
    String progress = "Connecting";
    for (int i = 0; i < dots; i++) progress += ".";
    showWiFiStatus("WiFi connection", progress.c_str(), "Please wait...");
    Serial.print(".");
    dots = (dots + 1) % 4;
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    showWiFiStatus("WiFi connected", WIFI_SSID, WiFi.localIP().toString().c_str());
    delay(1800);
  } else {
    Serial.println("\nWiFi connection timeout");
    showWiFiStatus("WiFi failed", "Check SSID/password", "Retry in 5 sec");
    delay(5000);
  }
}

void uploadToThingSpeak(int temperature, int humidity, int light) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  String url = "http://api.thingspeak.com/update?api_key=";
  url += THINGSPEAK_KEY;
  url += "&field1=";
  url += temperature;
  url += "&field2=";
  url += humidity;
  url += "&field3=";
  url += light;

  Serial.println("Uploading to ThingSpeak...");
  Serial.println(url);

  showWiFiStatus("ThingSpeak", "Uploading data...", "Please wait...");
  HTTPClient http;
  http.begin(url);
  lastUploadCode = http.GET();
  String response = http.getString();
  http.end();

  Serial.print("ThingSpeak HTTP code: ");
  Serial.println(lastUploadCode);
  Serial.print("ThingSpeak response: ");
  Serial.println(response);

  if (lastUploadCode == HTTP_CODE_OK && response != "0") {
    showWiFiStatus("ThingSpeak", "Upload complete", "Data sent OK");
  } else {
    showWiFiStatus("ThingSpeak", "Upload failed", "Check connection");
  }
  delay(1000);
}

void showSensorData(bool dhtOK) {
  oled.clearBuffer();
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.drawHLine(4, 32, 120);
  oled.drawVLine(64, 36, 24);

  drawThermometer(7, 6);
  drawDrop(8, 40);
  drawSun(72, 40);

  oled.setFont(u8g2_font_5x8_tf);
  oled.setCursor(28, 12);
  oled.print("TEMPERATURE");
  oled.setCursor(22, 43);
  oled.print("HUMIDITY");
  oled.setCursor(87, 43);
  oled.print("LIGHT");

  if (dhtOK) {
    drawValue(45, 29, lastTemperature, " C");
    drawValue(18, 61, lastHumidity, " %");
  } else {
    oled.setFont(u8g2_font_6x12_tf);
    oled.setCursor(45, 25);
    oled.print("ERROR");
  }
  drawValue(82, 61, lastLight, " %");
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  Wire.begin(SDA_PIN, SCL_PIN);
  oled.begin();
  oled.setFontPosBaseline();

  showWiFiStatus("ESP32 SENSOR", "Starting...", "Please wait...");
  connectWiFi();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int err = dht11.read(&temperature, &humidity, NULL);

  if (err == SimpleDHTErrSuccess) {
    lastTemperature = (int)temperature;
    lastHumidity = (int)humidity;
  } else {
    Serial.print("DHT11 read error: ");
    Serial.println(SimpleDHTErrCode(err));
  }

  int lightRaw = analogRead(LIGHT_PIN);
  lastLight = map(lightRaw, 0, 4095, 0, 100);
  lastLight = constrain(lastLight, 0, 100);
  showSensorData(err == SimpleDHTErrSuccess);

  if (lastUploadTime == 0 || millis() - lastUploadTime >= UPLOAD_INTERVAL) {
    uploadToThingSpeak(lastTemperature, lastHumidity, lastLight);
    lastUploadTime = millis();
  }

  delay(1000);
}
