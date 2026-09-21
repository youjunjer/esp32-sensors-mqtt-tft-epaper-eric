#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ESP32 Dev Module 腳位
const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// Wi-Fi
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Google Sheets
String sheetId = "1yvgQVsVGOBKeQtsRNEkea2NLekiC4g9jnulTyogUmek";
String sheetName = "data";
const char *GOOGLE_SCRIPT_ID = "AKfycbyR-Yp-uu4nIvnjvnkILaQ5AX8yFxp-UpBO-Sqs0su3ai1N_BvQsz_Q";
const unsigned long SHEET_INTERVAL = 10000UL;

// LINE Messaging API
const char *LINE_USER_ID = "YOUR_LINE_USER_ID";
const char *LINE_CHANNEL_ACCESS_TOKEN = "YOUR_LINE_CHANNEL_ACCESS_TOKEN";
const unsigned long LINE_INTERVAL = 30000UL;

const int TEMP_LIMIT = 28;
const int HUMIDITY_LIMIT = 70;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

int lastTemperature = 0;
int lastHumidity = 0;
int lastLight = 0;
unsigned long lastSheetTime = 0;
unsigned long lastLineTime = 0;

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
    showStatus("WiFi connection", progress.c_str(), "Please wait...");
    Serial.print(".");
    dots = (dots + 1) % 4;
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    String ip = WiFi.localIP().toString();
    showStatus("WiFi connected", WIFI_SSID, ip.c_str());
    delay(1500);
  } else {
    Serial.println("\nWiFi connection timeout");
    showStatus("WiFi failed", "Check SSID/password", "Retry later");
    delay(3000);
  }
}

String URLEncode(const char *msg) {
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";
  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') ||
        ('A' <= *msg && *msg <= 'Z') ||
        ('0' <= *msg && *msg <= '9')) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[(*msg >> 4) & 0x0F];
      encodedMsg += hex[*msg & 0x0F];
    }
    msg++;
  }
  return encodedMsg;
}

bool sendToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) return false;

  String data = String(lastTemperature) + "," + String(lastHumidity) + "," + String(lastLight);
  String encodedData = URLEncode(data.c_str());
  String encodedSheetName = URLEncode(sheetName.c_str());
  const char *host = "script.google.com";
  WiFiClientSecure sheetClient;
  sheetClient.setInsecure();
  sheetClient.setTimeout(5000);

  showStatus("Google Sheets", "Uploading data...", "Please wait...");
  if (!sheetClient.connect(host, 443)) {
    Serial.println("Google Sheets connection failed");
    showStatus("Google Sheets", "Upload failed", "Connection error");
    delay(800);
    return false;
  }

  String url = String("/macros/s/") + GOOGLE_SCRIPT_ID +
               "/exec?type=insert&dateInclude=1&sheetId=" + sheetId +
               "&sheetTag=" + encodedSheetName + "&data=" + encodedData;
  sheetClient.println(String("GET ") + url + " HTTP/1.1");
  sheetClient.println(String("Host: ") + host);
  sheetClient.println("Accept: */*");
  sheetClient.println("Connection: close");
  sheetClient.println();
  delay(100);
  sheetClient.stop();
  Serial.println("Google Sheets upload request sent");
  return true;
}

bool sendLineNotification() {
  String message = "環境異常警報！溫度: " + String(lastTemperature) +
                   " C，濕度: " + String(lastHumidity) + " %";
  String body = "{\"to\":\"" + String(LINE_USER_ID) +
                "\",\"messages\":[{\"type\":\"text\",\"text\":\"" +
                message + "\"}]}";

  showStatus("LINE notification", "Sending message...", "Abnormal data");
  Serial.println("Sending LINE notification...");
  WiFiClientSecure lineClient;
  lineClient.setInsecure();
  lineClient.setTimeout(5000);

  if (!lineClient.connect("api.line.me", 443)) {
    Serial.println("LINE connection failed");
    showStatus("LINE notification", "Send failed", "Connection error");
    delay(1000);
    return false;
  }

  lineClient.println("POST /v2/bot/message/push HTTP/1.1");
  lineClient.println("Connection: close");
  lineClient.println("Host: api.line.me");
  lineClient.println(String("Authorization: Bearer ") + LINE_CHANNEL_ACCESS_TOKEN);
  lineClient.println("Content-Type: application/json; charset=utf-8");
  lineClient.println(String("Content-Length: ") + body.length());
  lineClient.println();
  lineClient.println(body);
  lineClient.println();
  delay(100);
  lineClient.stop();

  Serial.println("LINE notification sent");
  showStatus("LINE notification", "Message sent", "Alert delivered");
  delay(1000);
  return true;
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
  oled.setCursor(24, 12);
  oled.print("TEMP");
  oled.setCursor(22, 44);
  oled.print("HUM");
  oled.setCursor(88, 44);
  oled.print("LIGHT");

  if (dhtOK) {
    drawValue(45, 29, lastTemperature, " C");
    drawBottomValue(24, 61, lastHumidity, " %");
  } else {
    oled.setFont(u8g2_font_6x12_tf);
    oled.setCursor(45, 25);
    oled.print("ERROR");
  }
  drawBottomValue(91, 61, lastLight, " %");
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(SDA_PIN, SCL_PIN);
  oled.begin();
  oled.setFontPosBaseline();
  showStatus("ESP32 SENSOR", "Starting...", "Please wait...");
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

  bool abnormal = lastTemperature > TEMP_LIMIT || lastHumidity > HUMIDITY_LIMIT;
  if (abnormal) {
    Serial.println("Abnormal condition detected");
    if (lastLineTime == 0 || millis() - lastLineTime >= LINE_INTERVAL) {
      sendLineNotification();
      lastLineTime = millis();
      showSensorData(err == SimpleDHTErrSuccess);
    }
  } else {
    lastLineTime = 0;
  }

  if (lastSheetTime == 0 || millis() - lastSheetTime >= SHEET_INTERVAL) {
    sendToGoogleSheets();
    lastSheetTime = millis();
    showSensorData(err == SimpleDHTErrSuccess);
  }

  delay(1000);
}
