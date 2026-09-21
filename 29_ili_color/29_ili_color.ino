#include <SPI.h>

// ILI9225 SPI 接線（無 SDO/MISO）
const int TFT_SCK  = 18;
const int TFT_MOSI = 23;
const int TFT_CS   = 27;
const int TFT_DC   = 26;  // RS / A0
const int TFT_RST  = 25;
#define SPI_FREQUENCY 15000000UL

const uint16_t TFT_WIDTH = 176;
const uint16_t TFT_HEIGHT = 220;
const unsigned long COLOR_TIME = 3000UL;

// ILI9225 要求 16 位元資料以高位元組在前傳送；使用兩次 8 位元傳送，
// 避免不同 ESP32 SPI 核心的 transfer16 位元組順序造成畫面雜訊。
void transfer16BE(uint16_t data) {
  SPI.transfer((uint8_t)(data >> 8));
  SPI.transfer((uint8_t)(data & 0xFF));
}

void writeCommand(uint8_t command) {
  digitalWrite(TFT_DC, LOW);
  digitalWrite(TFT_CS, LOW);
  SPI.transfer(command);
}

void writeData(uint16_t data) {
  digitalWrite(TFT_DC, HIGH);
  transfer16BE(data);
  digitalWrite(TFT_CS, HIGH);
}

void commandData(uint8_t command, uint16_t data) {
  writeCommand(command);
  writeData(data);
}

void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  commandData(0x36, x0);
  commandData(0x37, x1);
  commandData(0x38, y0);
  commandData(0x39, y1);
  commandData(0x20, x0);
  commandData(0x21, y0);
  writeCommand(0x22);
}

void fillScreen(uint16_t color) {
  setAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  for (uint32_t i = 0; i < (uint32_t)TFT_WIDTH * TFT_HEIGHT; i++) {
    transfer16BE(color);
  }
  digitalWrite(TFT_CS, HIGH);
}

void ili9225Init() {
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(100);

  commandData(0x01, 0x011C);
  commandData(0x02, 0x0100);
  commandData(0x03, 0x1030);
  commandData(0x08, 0x0808);
  commandData(0x0F, 0x0801);
  commandData(0x20, 0x0000);
  commandData(0x21, 0x0000);
  commandData(0x10, 0x0000);
  commandData(0x11, 0x1B41);
  commandData(0x12, 0x200E);
  commandData(0x13, 0x0D00);
  commandData(0x14, 0x0020);
  delay(50);
  commandData(0x10, 0x0F00);
  delay(50);
  commandData(0x11, 0x1B41);
  delay(50);
  commandData(0x12, 0x200E);
  commandData(0x13, 0x0D00);
  commandData(0x14, 0x0020);

  commandData(0x30, 0x0000);
  commandData(0x31, 0x00DB);
  commandData(0x32, 0x0000);
  commandData(0x33, 0x0000);
  commandData(0x34, 0x00DB);
  commandData(0x35, 0x0000);
  commandData(0x36, 0x00AF);
  commandData(0x37, 0x0000);
  commandData(0x38, 0x00DB);
  commandData(0x39, 0x0000);
  commandData(0x07, 0x1017);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  ili9225Init();
  SPI.endTransaction();

  Serial.println("ILI9225 color test ready");
}

void loop() {
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  fillScreen(0xF800); // 紅
  SPI.endTransaction();
  delay(COLOR_TIME);

  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  fillScreen(0x07E0); // 綠
  SPI.endTransaction();
  delay(COLOR_TIME);

  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  fillScreen(0x001F); // 藍
  SPI.endTransaction();
  delay(COLOR_TIME);

  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  fillScreen(0xFFFF); // 白
  SPI.endTransaction();
  delay(COLOR_TIME);

  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  fillScreen(0x0000); // 黑
  SPI.endTransaction();
  delay(COLOR_TIME);
}
