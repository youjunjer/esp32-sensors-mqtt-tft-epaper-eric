#include <Arduino.h>
#include <SPI.h>
#include "epd2in9b_V4.h"

// The V4 controller uses a native 128 x 296 buffer. The panel is used in
// landscape orientation, so the logical canvas below is 296 x 128.
constexpr int LOGICAL_WIDTH = 296;
constexpr int LOGICAL_HEIGHT = 128;
constexpr int EPD_BUFFER_BYTES = (EPD_WIDTH / 8) * EPD_HEIGHT;

uint8_t blackBuffer[EPD_BUFFER_BYTES];
uint8_t redBuffer[EPD_BUFFER_BYTES];
Epd epd;

// 5 x 7 font for the two words.
const uint8_t helloFont[5][5] = {
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x3E, 0x41, 0x41, 0x41, 0x3E}  // O
};
const uint8_t worldFont[5][5] = {
  {0x3F, 0x20, 0x78, 0x20, 0x3F}, // W
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x41, 0x41, 0x41, 0x3E}  // D
};

void setNativePixel(uint8_t* buffer, int nativeX, int nativeY, bool colored) {
  if (nativeX < 0 || nativeX >= EPD_WIDTH || nativeY < 0 || nativeY >= EPD_HEIGHT) {
    return;
  }
  const int index = nativeX + nativeY * EPD_WIDTH;
  const uint8_t mask = 0x80 >> (index % 8);
  if (colored) {
    buffer[index / 8] &= static_cast<uint8_t>(~mask);
  } else {
    buffer[index / 8] |= mask;
  }
}

// Convert landscape coordinates to the controller's native portrait layout.
// This mapping rotates the rendered landscape image by 180 degrees.
void setLandscapePixel(uint8_t* buffer, int x, int y, bool colored) {
  if (x < 0 || x >= LOGICAL_WIDTH || y < 0 || y >= LOGICAL_HEIGHT) {
    return;
  }
  const int nativeX = LOGICAL_HEIGHT - 1 - y;
  const int nativeY = x;
  setNativePixel(buffer, nativeX, nativeY, colored);
}

void drawWord(const uint8_t glyphs[5][5], int startY, bool blackText) {
  constexpr int scale = 4;
  constexpr int glyphWidth = 5;
  constexpr int spacing = 1;
  constexpr int textWidth = 5 * (glyphWidth + spacing) * scale - spacing * scale;
  const int startX = (LOGICAL_WIDTH - textWidth) / 2;

  for (int glyph = 0; glyph < 5; glyph++) {
    for (int col = 0; col < glyphWidth; col++) {
      for (int row = 0; row < 7; row++) {
        if ((glyphs[glyph][col] >> row) & 0x01) {
          for (int dx = 0; dx < scale; dx++) {
            for (int dy = 0; dy < scale; dy++) {
              const int x = startX + glyph * (glyphWidth + spacing) * scale + col * scale + dx;
              const int y = startY + row * scale + dy;
              // Remove the red layer for both words so the text is not red.
              setLandscapePixel(redBuffer, x, y, false);
              if (blackText) {
                setLandscapePixel(blackBuffer, x, y, true);
              }
            }
          }
        }
      }
    }
  }
}

void drawHello() {
  // Black plane starts white. For this V4 driver, 0x00 produces the red
  // background on the red/yellow plane after the driver's inversion.
  memset(blackBuffer, 0xFF, sizeof(blackBuffer));
  memset(redBuffer, 0x00, sizeof(redBuffer));

  drawWord(helloFont, 25, false); // white HELLO
  drawWord(worldFont, 75, true);  // black WORLD
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("36_epaper: red background, black HELLO");

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }

  drawHello();
  epd.Display(blackBuffer, redBuffer);
  Serial.println("Display complete");
  epd.Sleep();
}

void loop() {
  delay(1000);
}
