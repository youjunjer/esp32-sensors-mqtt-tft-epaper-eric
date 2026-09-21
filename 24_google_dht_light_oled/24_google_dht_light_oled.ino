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

// Wi-Fi 設定
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Google Sheet 設定
String sheetId = "1yvgQVsVGOBKeQtsRNEkea2NLekiC4g9jnulTyogUmek";
String sheetName = "data";
const char *GOOGLE_SCRIPT_ID = "AKfycbyR-Yp-uu4nIvnjvnkILaQ5AX8yFxp-UpBO-Sqs0su3ai1N_BvQsz_Q";
const unsigned long UPLOAD_INTERVAL = 10000UL;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

int lastTemperature = 0;
int lastHumidity = 0;
int lastLight = 0;
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

bool sendToGoogleSheets(int temperature, int humidity, int light) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return false;
  }

  String data = String(temperature) + "," + String(humidity) + "," + String(light);
  String encodedData = URLEncode(data.c_str());
  String encodedSheetName = URLEncode(sheetName.c_str());
  const char *host = "script.google.com";

  showStatus("Google Sheets", "Uploading data...", "Please wait...");
  Serial.println("Uploading to Google Sheets...");
  Serial.println("Data: " + data);

  WiFiClientSecure sheetClient;
  sheetClient.setInsecure();
  sheetClient.setTimeout(5000);

  if (!sheetClient.connect(host, 443)) {
    Serial.println("Google Sheets connection failed");
    showStatus("Google Sheets", "Upload failed", "Connection error");
    delay(1000);
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

  unsigned long waitStart = millis();
  while (sheetClient.connected() && millis() - waitStart < 3000UL) {
    while (sheetClient.available()) {
      String line = sheetClient.readStringUntil('\n');
      Serial.println(line);
      waitStart = millis();
    }
  }
  sheetClient.stop();

  Serial.println("Google Sheets upload request sent");
  showStatus("Google Sheets", "Upload complete", "Data sent OK");
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

  if (lastUploadTime == 0 || millis() - lastUploadTime >= UPLOAD_INTERVAL) {
    sendToGoogleSheets(lastTemperature, lastHumidity, lastLight);
    lastUploadTime = millis();
  }

  delay(1000);
}
