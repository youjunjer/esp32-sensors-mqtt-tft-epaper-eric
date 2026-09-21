#include <Wire.h>
#include <U8g2lib.h>
#include <SimpleDHT.h>

// ESP32 Dev Module 腳位設定
const int DHT_PIN = 14;
const int LIGHT_PIN = 33;
const int SDA_PIN = 21;
const int SCL_PIN = 22;

SimpleDHT11 dht11(DHT_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

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
  oled.setFont(u8g2_font_6x12_tf);
  oled.print(unit);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ESP32 ADC: 0~4095

  Wire.begin(SDA_PIN, SCL_PIN);
  oled.begin();
  oled.setFont(u8g2_font_6x12_tf);
  oled.setFontPosBaseline();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int err = dht11.read(&temperature, &humidity, NULL);

  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);

  oled.clearBuffer();
  // 儀表板外框與上下、左右分區
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.drawHLine(4, 32, 120);
  oled.drawVLine(64, 36, 24);

  // 上方：溫度
  drawThermometer(7, 6);
  oled.setFont(u8g2_font_5x8_tf);
  oled.setCursor(28, 12);
  oled.print("TEMPERATURE");

  // 下方左：濕度；下方右：亮度
  drawDrop(8, 40);
  drawSun(72, 40);
  oled.setCursor(22, 43);
  oled.print("HUMIDITY");
  oled.setCursor(87, 43);
  oled.print("LIGHT");

  if (err == SimpleDHTErrSuccess) {
    drawValue(45, 29, (int)temperature, " C");
    drawValue(18, 61, (int)humidity, " %");
    drawValue(82, 61, lightPercent, " %");

    Serial.print("Temperature: ");
    Serial.print((int)temperature);
    Serial.print(" C, Humidity: ");
    Serial.print((int)humidity);
    Serial.println(" %");
  } else {
    oled.setFont(u8g2_font_6x12_tf);
    oled.setCursor(45, 25);
    oled.print("ERROR");
    Serial.print("DHT11 read error: ");
    Serial.println(SimpleDHTErrCode(err));
  }

  if (err != SimpleDHTErrSuccess) {
    drawValue(82, 61, lightPercent, " %");
  }
  oled.sendBuffer();

  Serial.print("Light raw: ");
  Serial.print(lightRaw);
  Serial.print(", Light: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  delay(1000);
}
