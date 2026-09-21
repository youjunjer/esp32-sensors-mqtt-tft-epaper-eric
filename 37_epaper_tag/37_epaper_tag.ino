#include <Arduino.h>
#include <SPI.h>
#include "epd2in9b_V4.h"
#include "midautumn_bitmap.h"

constexpr int LOGICAL_WIDTH = 296;
constexpr int LOGICAL_HEIGHT = 128;
constexpr int EPD_BUFFER_BYTES = (EPD_WIDTH / 8) * EPD_HEIGHT;

uint8_t blackBuffer[EPD_BUFFER_BYTES];
uint8_t redBuffer[EPD_BUFFER_BYTES];
Epd epd;

// Small 5 x 7 uppercase font used by the price-tag layout.
void glyph(char input, uint8_t out[5]) {
  const char c = (input >= 'a' && input <= 'z') ? input - ('a' - 'A') : input;
  const uint8_t* p = nullptr;
  static const uint8_t blank[5] = {0, 0, 0, 0, 0};
  static const uint8_t A[5] = {0x7E,0x09,0x09,0x09,0x7E};
  static const uint8_t B[5] = {0x7F,0x49,0x49,0x49,0x36};
  static const uint8_t C[5] = {0x3E,0x41,0x41,0x41,0x22};
  static const uint8_t D[5] = {0x7F,0x41,0x41,0x22,0x1C};
  static const uint8_t E[5] = {0x7F,0x49,0x49,0x49,0x41};
  static const uint8_t I[5] = {0x00,0x41,0x7F,0x41,0x00};
  static const uint8_t K[5] = {0x7F,0x08,0x14,0x22,0x41};
  static const uint8_t L[5] = {0x7F,0x40,0x40,0x40,0x40};
  static const uint8_t O[5] = {0x3E,0x41,0x41,0x41,0x3E};
  static const uint8_t P[5] = {0x7F,0x09,0x09,0x09,0x06};
  static const uint8_t R[5] = {0x7F,0x09,0x19,0x29,0x46};
  static const uint8_t S[5] = {0x46,0x49,0x49,0x49,0x31};
  static const uint8_t T[5] = {0x01,0x01,0x7F,0x01,0x01};
  static const uint8_t Y[5] = {0x01,0x02,0x7C,0x02,0x01};
  static const uint8_t G[5] = {0x3E,0x41,0x49,0x49,0x7A};
  static const uint8_t H[5] = {0x7F,0x08,0x08,0x08,0x7F};
  static const uint8_t M[5] = {0x7F,0x02,0x0C,0x02,0x7F};
  static const uint8_t N[5] = {0x7F,0x04,0x08,0x10,0x7F};
  static const uint8_t U[5] = {0x3F,0x40,0x40,0x40,0x3F};
  static const uint8_t V[5] = {0x1F,0x20,0x40,0x20,0x1F};
  static const uint8_t W[5] = {0x7F,0x20,0x18,0x20,0x7F};
  static const uint8_t X[5] = {0x63,0x14,0x08,0x14,0x63};
  static const uint8_t zero[5] = {0x3E,0x45,0x49,0x51,0x3E};
  static const uint8_t one[5]  = {0x00,0x42,0x7F,0x40,0x00};
  static const uint8_t two[5]  = {0x62,0x51,0x49,0x49,0x46};
  static const uint8_t three[5]= {0x22,0x41,0x49,0x49,0x36};
  static const uint8_t four[5] = {0x18,0x14,0x12,0x7F,0x10};
  static const uint8_t five[5] = {0x2F,0x49,0x49,0x49,0x31};
  static const uint8_t six[5]  = {0x3E,0x49,0x49,0x49,0x32};
  static const uint8_t seven[5]= {0x01,0x71,0x09,0x05,0x03};
  static const uint8_t eight[5]= {0x36,0x49,0x49,0x49,0x36};
  static const uint8_t nine[5] = {0x26,0x49,0x49,0x49,0x3E};
  static const uint8_t colon[5]= {0x00,0x36,0x36,0x00,0x00};
  static const uint8_t dot[5]  = {0x00,0x60,0x60,0x00,0x00};
  static const uint8_t dollar[5]= {0x24,0x2A,0x7F,0x2A,0x12};
  switch (c) {
    case 'A':p=A;break; case 'B':p=B;break; case 'C':p=C;break; case 'D':p=D;break;
    case 'E':p=E;break; case 'G':p=G;break; case 'H':p=H;break; case 'I':p=I;break; case 'K':p=K;break; case 'L':p=L;break;
    case 'M':p=M;break; case 'N':p=N;break;
    case 'O':p=O;break; case 'P':p=P;break; case 'R':p=R;break; case 'S':p=S;break;
    case 'T':p=T;break; case 'U':p=U;break; case 'V':p=V;break; case 'W':p=W;break; case 'X':p=X;break; case 'Y':p=Y;break; case '0':p=zero;break; case '1':p=one;break;
    case '2':p=two;break; case '3':p=three;break; case '4':p=four;break; case '5':p=five;break;
    case '6':p=six;break; case '7':p=seven;break; case '8':p=eight;break; case '9':p=nine;break;
    case ':':p=colon;break; case '.':p=dot;break; case '$':p=dollar;break; default:p=blank;break;
  }
  memcpy(out, p, 5);
}

void setNativePixel(uint8_t* buffer, int nativeX, int nativeY, bool colored) {
  if (nativeX < 0 || nativeX >= EPD_WIDTH || nativeY < 0 || nativeY >= EPD_HEIGHT) return;
  const int index = nativeX + nativeY * EPD_WIDTH;
  const uint8_t mask = 0x80 >> (index % 8);
  if (colored) buffer[index / 8] &= static_cast<uint8_t>(~mask);
  else buffer[index / 8] |= mask;
}

// Keep the same 180-degree orientation as the previous 36_epaper screen.
void setPixel(uint8_t* buffer, int x, int y, bool colored) {
  if (x < 0 || x >= LOGICAL_WIDTH || y < 0 || y >= LOGICAL_HEIGHT) return;
  setNativePixel(buffer, LOGICAL_HEIGHT - 1 - y, x, colored);
}

void fillRect(uint8_t* buffer, int x, int y, int w, int h, bool colored) {
  for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) setPixel(buffer, xx, yy, colored);
}

void drawLine(uint8_t* buffer, int x0, int y0, int x1, int y1, bool colored) {
  const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    setPixel(buffer, x0, y0, colored);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void fillCircle(uint8_t* buffer, int cx, int cy, int radius, bool colored) {
  for (int y = -radius; y <= radius; y++) for (int x = -radius; x <= radius; x++)
    if (x * x + y * y <= radius * radius) setPixel(buffer, cx + x, cy + y, colored);
}

void drawText(const char* text, int x, int y, int scale, bool black, bool red) {
  int cursor = x;
  uint8_t data[5];
  while (*text) {
    glyph(*text++, data);
    for (int col = 0; col < 5; col++) for (int row = 0; row < 7; row++) if (data[col] & (1 << row))
      for (int dx = 0; dx < scale; dx++) for (int dy = 0; dy < scale; dy++) {
        setPixel(blackBuffer, cursor + col * scale + dx, y + row * scale + dy, black);
        setPixel(redBuffer, cursor + col * scale + dx, y + row * scale + dy, red);
      }
    cursor += 6 * scale;
  }
}

void drawBarcode() {
  const uint8_t bars[] = {2,1,1,3,2,1,3,1,1,2,1,3,2,2,1,1,3,1,2,1,3,2,1,1,2,3,1,2,1,3,2,1};
  int x = 10;
  for (uint8_t i = 0; i < sizeof(bars); i++) {
    const int width = bars[i];
    if ((i & 1) == 0) fillRect(blackBuffer, x, 101, width, 18, true);
    x += width + 1;
  }
  drawText("1234567890", 10, 120, 1, true, false);
}

void drawMidAutumn() {
  for (int y = 0; y < MIDAUTUMN_H; y++) {
    for (int x = 0; x < MIDAUTUMN_W; x++) {
      const int index = y * (MIDAUTUMN_W / 8) + x / 8;
      const uint8_t mask = 0x80 >> (x % 8);
      if (pgm_read_byte(&MIDAUTUMN_BLACK[index]) & mask) setPixel(blackBuffer, x, y, true);
      if (pgm_read_byte(&MIDAUTUMN_RED[index]) & mask) setPixel(redBuffer, x, y, true);
    }
  }
}

void drawTag() {
  // Mid-Autumn Festival three-color illustration.
  memset(blackBuffer, 0xFF, sizeof(blackBuffer));
  memset(redBuffer, 0xFF, sizeof(redBuffer));
  drawMidAutumn();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("37_epaper_tag");
  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }
  drawTag();
  epd.Display(blackBuffer, redBuffer);
  Serial.println("Tag display complete");
  epd.Sleep();
}

void loop() {
  delay(1000);
}
