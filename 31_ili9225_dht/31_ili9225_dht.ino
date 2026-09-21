#include <SPI.h>
#include <SimpleDHT.h>

// ILI9225 SPI（無 SDO/MISO）
const int TFT_SCK  = 18;
const int TFT_MOSI = 23;
const int TFT_CS   = 27;
const int TFT_DC   = 26;  // RS / A0
const int TFT_RST  = 25;
const int DHT_PIN  = 14;

#define SPI_FREQUENCY 15000000UL
const int SCREEN_W = 176;
const int SCREEN_H = 220;
const unsigned long SENSOR_INTERVAL = 2000UL;

// RGB565 顏色
const uint16_t NAVY       = 0x10A2;
const uint16_t PANEL      = 0x2145;
const uint16_t PANEL_EDGE = 0x39C7;
const uint16_t WHITE      = 0xFFFF;
const uint16_t SOFT_WHITE = 0xD6BA;
const uint16_t RED        = 0xF8A6;
const uint16_t ORANGE     = 0xFD20;
const uint16_t BLUE       = 0x45BF;
const uint16_t CYAN       = 0x5DFF;
const uint16_t YELLOW     = 0xFFE0;

SimpleDHT11 dht11(DHT_PIN);
int temperature = 0;
int humidity = 0;
bool sensorOK = false;
unsigned long lastRead = 0;

void transfer16BE(uint16_t data) {
  SPI.transfer((uint8_t)(data >> 8));
  SPI.transfer((uint8_t)data);
}

void command(uint8_t value) {
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, LOW);
  SPI.transfer(value);
  digitalWrite(TFT_DC, HIGH);
}

void data16(uint16_t value) {
  transfer16BE(value);
  digitalWrite(TFT_CS, HIGH);
}

void commandData(uint8_t cmd, uint16_t value) {
  command(cmd);
  data16(value);
}

void setWindow(int x0, int y0, int x1, int y1) {
  commandData(0x36, x0);
  commandData(0x37, x1);
  commandData(0x38, y0);
  commandData(0x39, y1);
  commandData(0x20, x0);
  commandData(0x21, y0);
  command(0x22);
}

void fillRect(int x, int y, int w, int h, uint16_t color) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SCREEN_W) w = SCREEN_W - x;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (w <= 0 || h <= 0) return;
  setWindow(x, y, x + w - 1, y + h - 1);
  for (long i = 0; i < (long)w * h; i++) transfer16BE(color);
  digitalWrite(TFT_CS, HIGH);
}

void drawPixel(int x, int y, uint16_t color) {
  if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
  fillRect(x, y, 1, 1, color);
}

void drawHLine(int x, int y, int w, uint16_t color) { fillRect(x, y, w, 1, color); }
void drawVLine(int x, int y, int h, uint16_t color) { fillRect(x, y, 1, h, color); }

void drawCircle(int cx, int cy, int r, uint16_t color) {
  int x = r, y = 0, err = 0;
  while (x >= y) {
    drawPixel(cx + x, cy + y, color); drawPixel(cx + y, cy + x, color);
    drawPixel(cx - y, cy + x, color); drawPixel(cx - x, cy + y, color);
    drawPixel(cx - x, cy - y, color); drawPixel(cx - y, cy - x, color);
    drawPixel(cx + y, cy - x, color); drawPixel(cx + x, cy - y, color);
    y++;
    if (err <= 0) err += 2 * y + 1;
    if (err > 0) { x--; err -= 2 * x + 1; }
  }
}

void fillCircle(int cx, int cy, int r, uint16_t color) {
  for (int y = -r; y <= r; y++) {
    int half = (int)sqrt((float)r * r - (float)y * y);
    fillRect(cx - half, cy + y, half * 2 + 1, 1, color);
  }
}

void getGlyph(char c, uint8_t g[5]) {
  memset(g, 0, 5);
  switch (c) {
    case '0': { uint8_t a[5]={0x3E,0x51,0x49,0x45,0x3E}; memcpy(g,a,5); break; }
    case '1': { uint8_t a[5]={0x00,0x42,0x7F,0x40,0x00}; memcpy(g,a,5); break; }
    case '2': { uint8_t a[5]={0x42,0x61,0x51,0x49,0x46}; memcpy(g,a,5); break; }
    case '3': { uint8_t a[5]={0x21,0x41,0x45,0x4B,0x31}; memcpy(g,a,5); break; }
    case '4': { uint8_t a[5]={0x18,0x14,0x12,0x7F,0x10}; memcpy(g,a,5); break; }
    case '5': { uint8_t a[5]={0x27,0x45,0x45,0x45,0x39}; memcpy(g,a,5); break; }
    case '6': { uint8_t a[5]={0x3C,0x4A,0x49,0x49,0x30}; memcpy(g,a,5); break; }
    case '7': { uint8_t a[5]={0x01,0x71,0x09,0x05,0x03}; memcpy(g,a,5); break; }
    case '8': { uint8_t a[5]={0x36,0x49,0x49,0x49,0x36}; memcpy(g,a,5); break; }
    case '9': { uint8_t a[5]={0x06,0x49,0x49,0x29,0x1E}; memcpy(g,a,5); break; }
    case 'T': { uint8_t a[5]={0x01,0x01,0x7F,0x01,0x01}; memcpy(g,a,5); break; }
    case 'E': { uint8_t a[5]={0x7F,0x49,0x49,0x49,0x41}; memcpy(g,a,5); break; }
    case 'M': { uint8_t a[5]={0x7F,0x02,0x0C,0x02,0x7F}; memcpy(g,a,5); break; }
    case 'P': { uint8_t a[5]={0x7F,0x09,0x09,0x09,0x06}; memcpy(g,a,5); break; }
    case 'H': { uint8_t a[5]={0x7F,0x08,0x08,0x08,0x7F}; memcpy(g,a,5); break; }
    case 'U': { uint8_t a[5]={0x3F,0x40,0x40,0x40,0x3F}; memcpy(g,a,5); break; }
    case 'I': { uint8_t a[5]={0x00,0x41,0x7F,0x41,0x00}; memcpy(g,a,5); break; }
    case 'D': { uint8_t a[5]={0x7F,0x41,0x41,0x22,0x1C}; memcpy(g,a,5); break; }
    case 'Y': { uint8_t a[5]={0x07,0x08,0x70,0x08,0x07}; memcpy(g,a,5); break; }
    case 'C': { uint8_t a[5]={0x3E,0x41,0x41,0x41,0x22}; memcpy(g,a,5); break; }
    case '%': { uint8_t a[5]={0x63,0x13,0x08,0x64,0x63}; memcpy(g,a,5); break; }
    case '-': { uint8_t a[5]={0x08,0x08,0x08,0x08,0x08}; memcpy(g,a,5); break; }
  }
}

void drawText(const char *text, int x, int y, int scale, uint16_t color) {
  int cursor = x;
  while (*text) {
    uint8_t glyph[5]; getGlyph(*text++, glyph);
    for (int col = 0; col < 5; col++) {
      for (int row = 0; row < 7; row++) {
        if (glyph[col] & (1 << row)) fillRect(cursor + col * scale, y + row * scale, scale, scale, color);
      }
    }
    cursor += 6 * scale;
  }
}

void drawThermometer(int cx, int cy) {
  fillRect(cx - 4, cy - 22, 8, 28, RED);
  fillCircle(cx, cy + 9, 10, RED);
  fillRect(cx - 2, cy - 16, 4, 25, ORANGE);
  drawCircle(cx, cy + 9, 11, SOFT_WHITE);
  drawCircle(cx, cy - 22, 5, SOFT_WHITE);
}

void drawDrop(int cx, int cy) {
  for (int y = -23; y <= 10; y++) {
    int width = (y < 0) ? max(1, (y + 23) / 3) : max(1, 10 - y / 2);
    fillRect(cx - width, cy + y, width * 2 + 1, 1, BLUE);
  }
  drawCircle(cx, cy + 2, 9, CYAN);
  fillCircle(cx - 3, cy - 8, 2, WHITE);
}

void drawScreen() {
  fillRect(0, 0, SCREEN_W, SCREEN_H, NAVY);
  fillRect(0, 0, SCREEN_W, 7, CYAN);
  drawText("TEMP", 14, 17, 2, SOFT_WHITE);
  drawText("HUMIDITY", 95, 17, 1, SOFT_WHITE);
  drawHLine(12, 35, 152, PANEL_EDGE);

  fillRect(10, 48, 156, 72, PANEL);
  drawHLine(10, 48, 156, RED);
  drawThermometer(31, 80);
  drawText("TEMP", 52, 59, 1, RED);
  if (sensorOK) {
    char value[8];
    itoa(temperature, value, 10);
    drawText(value, 55, 78, 4, WHITE);
    drawText("C", 125, 91, 2, ORANGE);
  } else drawText("-- C", 58, 82, 2, WHITE);

  fillRect(10, 132, 156, 72, PANEL);
  drawHLine(10, 132, 156, BLUE);
  drawDrop(31, 169);
  drawText("HUMIDITY", 52, 143, 1, CYAN);
  if (sensorOK) {
    char value[8];
    itoa(humidity, value, 10);
    drawText(value, 55, 162, 4, WHITE);
    drawText("%", 125, 175, 2, CYAN);
  } else drawText("-- %", 58, 166, 2, WHITE);
}

void ili9225Init() {
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_RST, LOW); delay(50);
  digitalWrite(TFT_RST, HIGH); delay(100);
  commandData(0x01, 0x011C); commandData(0x02, 0x0100);
  commandData(0x03, 0x1030); commandData(0x08, 0x0808);
  commandData(0x0F, 0x0801); commandData(0x20, 0x0000);
  commandData(0x21, 0x0000); commandData(0x10, 0x0000);
  commandData(0x11, 0x1B41); commandData(0x12, 0x200E);
  commandData(0x13, 0x0D00); commandData(0x14, 0x0020);
  delay(50); commandData(0x10, 0x0F00); delay(50);
  commandData(0x11, 0x1B41); commandData(0x12, 0x200E);
  commandData(0x13, 0x0D00); commandData(0x14, 0x0020);
  commandData(0x30, 0x0000); commandData(0x31, 0x00DB);
  commandData(0x32, 0x0000); commandData(0x33, 0x0000);
  commandData(0x34, 0x00DB); commandData(0x35, 0x0000);
  commandData(0x36, 0x00AF); commandData(0x37, 0x0000);
  commandData(0x38, 0x00DB); commandData(0x39, 0x0000);
  commandData(0x07, 0x1017); delay(50);
}

void readSensor() {
  byte temp = 0, hum = 0;
  int err = dht11.read(&temp, &hum, NULL);
  sensorOK = (err == SimpleDHTErrSuccess);
  if (sensorOK) { temperature = temp; humidity = hum; }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_CS, OUTPUT); pinMode(TFT_DC, OUTPUT); pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  ili9225Init();
  readSensor();
  drawScreen();
  SPI.endTransaction();
  lastRead = millis();
}

void loop() {
  if (millis() - lastRead >= SENSOR_INTERVAL) {
    lastRead = millis();
    readSensor();
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    drawScreen();
    SPI.endTransaction();
    Serial.print("Temperature: "); Serial.print(temperature);
    Serial.print(" C, Humidity: "); Serial.print(humidity); Serial.println(" %");
  }
  delay(10);
}
