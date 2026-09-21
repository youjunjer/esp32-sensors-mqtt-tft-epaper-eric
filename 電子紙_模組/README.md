# Waveshare 2.9 吋三色電子紙 V4（ESP32）

本資料夾包含微雪官方 `epd2in9b_V4` Arduino 驅動，已改成目前 ESP32 Dev Module 的腳位配置。

## 模組規格

- 2.9 吋三色電子紙：黑／白／紅
- 顯示解析度：296 × 128；官方驅動以 `128 × 296` 原生方向傳送資料
- 驅動版本：Rev2.1／模組 V4
- 介面：SPI

## ESP32 接線

| 電子紙腳位 | ESP32 Dev Module | 說明 |
|---|---:|---|
| VCC | 3V3 | 僅使用 3.3 V |
| GND | GND | 共地 |
| DIN／MOSI | GPIO23 | SPI MOSI |
| CLK／SCK | GPIO18 | SPI Clock |
| CS | GPIO27 | 片選 |
| DC | GPIO26 | 資料／命令選擇 |
| RST | GPIO25 | 硬體重置 |
| BUSY | GPIO34 | 電子紙忙碌狀態；ESP32 輸入專用腳位 |

`PWR_PIN` 在驅動中保留為 GPIO32，電子紙模組不需接此腳位；電子紙直接接 3V3。不要把 VCC 接 5V。

## 目前專案腳位相容性

- DHT11：GPIO14
- 光敏電阻：GPIO33
- 綠／黃／紅 LED：GPIO15／GPIO2／GPIO4
- GPIO0：換頁按鍵
- 電子紙占用：GPIO18、23、25、26、27、34

## 程式庫

此官方驅動只依賴 Arduino 內建的 `SPI.h`，不需要額外安裝通用電子紙函式庫。專案目前已有的 `PubSubClient`、`ArduinoJson`、`SimpleDHT`、`Adafruit_GFX` 可繼續使用；電子紙驅動本身不依賴 Adafruit GFX。

官方來源：<https://github.com/waveshareteam/e-Paper/tree/master/Arduino/epd2in9b_V4>
