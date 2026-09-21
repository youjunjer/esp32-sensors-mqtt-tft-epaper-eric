# ESP32 Sensors, MQTT, TFT & E-Paper

這是 ESP32 感測器與顯示器實習專案，收錄從 GPIO、LED、按鍵、類比輸入開始，到 DHT 溫濕度、光線感測、Wi‑Fi、MQTT、TFT ILI9225 與 2.9 吋三色電子紙的逐步範例。

## 內容

- `01_hello`–`08_RGBCycle`：Arduino/ESP32 基礎練習
- `21_wifi_pm25_json_OLED_DHT`：Wi‑Fi、PM2.5 API、OLED 與 DHT
- `22_dht_light_oled`–`25_dht_line`：感測器、OLED、ThingSpeak、Google/LINE 整合
- `26_mqtt`、`27_mqtt_ctrl`：MQTT 感測資料與控制
- `29_ili_color`、`31_ili9225_dht`–`35_ili_mqtt_ctrl_page`：ILI9225 TFT 顯示與 MQTT 控制介面
- `36_epaper`–`40_epaper_mqtt`：電子紙顯示、標籤、月亮、感測器與 MQTT
- `電子紙_模組`：電子紙驅動模組原始碼
- `成果照片` 與圖檔：專案成果與系統架構素材

## 使用方式

1. 安裝 Arduino IDE 或 Arduino CLI，選擇相容的 ESP32 board package。
2. 依各範例資料夾名稱開啟對應的 `.ino` 檔案。
3. 安裝程式所需函式庫，例如 DHT sensor library、PubSubClient、Adafruit GFX、OLED/TFT 函式庫。
4. 上傳前，將程式中的 Wi‑Fi、MQTT、ThingSpeak、LINE 或政府資料 API placeholder 改成自己的設定。
5. 依硬體接線與顯示器型號調整 GPIO 腳位；不同模組可能需要不同的 SPI 腳位與初始化參數。

## 公開 repository 的安全提醒

本 repository 刻意不包含實際的 Wi‑Fi 密碼、服務 token 或 API key。所有範例中的敏感設定已替換為 placeholder；請勿將真實憑證提交到 GitHub。若任何憑證曾經被公開，請立即到對應服務撤銷並重新產生。

## 授權

目前未指定額外開源授權；如需允許他人重用程式碼，建議另行加入適合的 LICENSE。
