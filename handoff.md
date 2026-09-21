# ESP32 物聯網專案交接文件

更新日期：2026-08-31

## 一、硬體設定

- 開發板：ESP32 Dev Module
- USB 序列埠：COM8
- DHT11：GPIO14
- 光敏電阻：GPIO33，ADC 讀值轉換為 0–100%
- OLED SSD1306：I²C
  - SDA：GPIO21
  - SCL：GPIO22

## 二、目前最新專案

目前使用版本：`27_mqtt_ctrl`

程式檔案：

`D:\esp32實習\27_mqtt_ctrl\27_mqtt_ctrl.ino`

功能：

- 以 MQTT callback 訂閱控制指令，控制三顆 LED：綠燈 GPIO15（電燈）、黃燈 GPIO2（風扇）、紅燈 GPIO4（除濕機）。
- MQTT Broker：`mqttgo.io`，Port：1883，不使用帳號密碼。
- 訂閱主題：`eric/class305/ctrl`。
- 指令格式：`{"gled":"on"}`、`{"yled":"off"}`、`{"rled":"on"}`、`{"rled":"off"}`。
- 收到 MQTT 指令後，OLED 顯示指令內容 1 秒，再回到感測資料畫面。
- DHT11 與光敏電阻每 10 秒讀取一次，OLED 感測資料每 10 秒更新一次。
- Client ID 使用 ESP32 晶片 MAC 加上隨機值產生。
- 保留原本 DHT11 GPIO14、光敏電阻 GPIO33 與 OLED 顯示功能。

目前狀態：

- 程式已建立並完成 MQTT callback、JSON 指令解析與 LED 控制邏輯。
- 目前 Arduino CLI 編譯會卡在 ESP32 編譯階段，只產生 bootloader，尚未產生完整 `.bin`。
- 因此 `27_mqtt_ctrl` 尚未成功燒錄至 COM8，需後續排除 Arduino IDE／ESP32 編譯環境問題。

上一版：`25_dht_line`

程式檔案：

`D:\esp32實習\25_dht_line\25_dht_line.ino`

功能：

- OLED 顯示溫度、濕度、亮度。
- OLED 使用溫度計、水滴、太陽圖示。
- 溫度使用大字顯示。
- 濕度與亮度採用圖示在左、數值在右的排列方式。
- DHT11 與光敏電阻資料每秒讀取一次。
- Google Sheets 每 10 秒上傳一筆資料。
- 溫度大於 28°C 或濕度大於 70% 時觸發 LINE 通知。
- 異常持續期間，每 30 秒最多發送一次 LINE 通知。
- Wi‑Fi 連線、Google Sheets 上傳及 LINE 通知時，OLED 顯示處理狀態。
- 最新版本已使用 Arduino CLI 編譯並成功燒錄至 COM7。

## 三、雲端服務設定

### Google Sheets

- 工作表名稱：`data`
- 寫入欄位：時間、溫度、濕度、亮度
- 透過 Google Apps Script HTTPS GET 方式上傳。

### LINE Messaging API

- 使用 LINE Messaging API Push Message。
- 已設定指定使用者與 Channel Access Token。
- 警報條件：溫度 > 28°C 或濕度 > 70%。
- Wi‑Fi 密碼、Google Apps Script 識別資訊及 LINE Token 已寫入程式碼。
- 若程式要公開分享，應先移除敏感資訊，並建議重新產生 LINE Token。

## 四、歷代專案

- `22_dht_light_oled`：OLED 顯示溫度、濕度與亮度。
- `23_dht_light_oled_thingspeak`：上傳 ThingSpeak。
- `24_google_dht_light_oled`：上傳 Google Sheets。
- `25_dht_line`：Google Sheets 加上 LINE 異常通知，目前最新版本。
- `26_mqtt`：透過 MQTT 每 10 秒發布溫度、濕度與亮度 JSON 資料。
- `27_mqtt_ctrl`：目前最新版本，透過 MQTT callback 訂閱並控制三顆 LED。

## 五、I²C 改線備註

- 目前程式仍使用 OLED SDA GPIO21、SCL GPIO22。
- 若硬體只能使用 GPIO12、GPIO13，建議 GPIO13 接 SDA、GPIO12 接 SCL。
- 程式需改為：`const int SDA_PIN = 13;`、`const int SCL_PIN = 12;`，並維持 `Wire.begin(SDA_PIN, SCL_PIN);`。
- GPIO12 是 ESP32 開機 strapping 腳位，可能造成開機問題；需確認 OLED I²C 上拉電壓為 3.3V。

## 六、成果報告

原始報告：

`D:\esp32實習\0810物聯網成果報告.docx`

使用 imagegen 重新製作架構圖與流程圖的版本：

`D:\esp32實習\0810物聯網成果報告_imagegen.docx`

原始 Word 檔曾因被 Word 開啟而鎖定，因此更新版另存為 `_imagegen` 檔名。

架構圖：

`D:\esp32實習\架構圖_imagegen.png`

流程圖：

`D:\esp32實習\流程圖_imagegen.png`

成果照片資料夾：

`D:\esp32實習\成果照片`

內容包含：

- ESP32 實體接線照片
- OLED 顯示畫面
- LINE 異常通知截圖
- Google Sheets 資料截圖
- Google Sheets 圖表截圖

## 七、最近一次 OLED 調整

曾透過 OpenCV 擷取 webcam 畫面檢查 OLED，發現文字與圖示互相重疊，因此進行以下調整：

- 將 `TEMPERATURE` 改為 `TEMP`。
- 將 `HUMIDITY` 改為 `HUM`。
- 縮小溫度計、水滴與太陽圖示。
- 濕度與亮度改為圖示在左、數值在右。
- 底部濕度與亮度使用較小字體。
- 調整後已重新編譯並成功燒錄。

## 八、後續建議

1. 排除 Arduino CLI／ESP32 編譯環境問題，完成 `27_mqtt_ctrl` 編譯。
2. 編譯成功後燒錄至 COM8，確認序列埠輸出與 MQTT 連線。
3. 以 MQTT 發送各種 JSON 指令，確認三顆 LED 與 OLED 指令顯示。
4. 確認感測器與 OLED 是否確實每 10 秒更新一次。
5. 若改用 GPIO12/13，先確認 ESP32 能正常重新開機。
6. 若要公開程式碼或報告，移除 Wi‑Fi 密碼、LINE Token、Google Sheet ID 等敏感資訊。

## 九、2026-09-14 最新交接紀錄

### 目前主要版本

目前以：

`D:\esp32實習\35_ili_mqtt_ctrl_page\35_ili_mqtt_ctrl_page.ino`

作為最新開發版本。

功能包含：

- ILI9225 176×220 TFT 顯示。
- 使用 Adafruit GFX 進行畫面繪製，並搭配自訂 ILI9225 SPI 底層驅動。
- DHT11 溫度、濕度：GPIO14。
- 光敏電阻亮度：GPIO33，轉換為 0–100%。
- MQTT 發布溫度、濕度、亮度：每 10 秒一次。
- MQTT 資料主題：`eric/class302/data`。
- MQTT 訂閱控制主題：`eric/class302/ctrl`。
- 使用 ArduinoJson 搭配 MQTT callback 解析控制指令。
- 綠燈 GPIO15、黃燈 GPIO2、紅燈 GPIO4。
- GPIO0 按鍵換頁，按鍵另一端接 GND，使用 `INPUT_PULLUP`。
- NTP 同步台灣時間 UTC+8，格式如 `09/14 Mon 13:20`。

### 35 版本頁面

- 第 1 頁：溫度、濕度、亮度總覽、Wi‑Fi／MQTT 狀態列、日期時間。
- 第 2 頁：溫度半圓 Gauge 與最近 10 分鐘溫度折線圖。
- 第 3 頁：濕度半圓 Gauge 與最近 10 分鐘濕度折線圖。
- 第 4 頁：亮度半圓 Gauge 與最近 10 分鐘亮度折線圖。
- 每 10 秒取樣一次，最多保存 60 筆資料。
- Gauge 使用圓形色塊與中央白色圓形：
  - 溫度：10–20 綠色、20–30 黃色、30–40 紅色。
  - 濕度：0–40 綠色、40–70 黃色、70–100 紅色。
  - 亮度：0–30 藍色、30–70 黃色、70–100 綠色。

### 35 版本 ILI9225 腳位

- SCK：GPIO18
- MOSI／SDI：GPIO23
- CS：GPIO27
- RS／DC：GPIO26
- RST：GPIO25
- SDO／MISO：未接

### MQTT 設定

- Wi‑Fi SSID：`a`
- Wi‑Fi 密碼：`12345678`
- Broker：`mqttgo.io`
- Port：`1883`
- 發布 JSON 格式：`{"temp":25,"humi":65,"light":95}`
- 控制 JSON 範例：`{"gled":"on"}`、`{"yled":"off"}`、`{"rled":"on"}`

### 已安裝函式庫

- Adafruit GFX Library
- PubSubClient
- ArduinoJson
- SimpleDHT

Arduino IDE 目前可能找到多個函式庫副本，優先使用：

`C:\Users\user\Documents\Arduino\libraries`

### 最近排除的編譯問題

- 曾因重複定義 `drawHumidityPage()` 造成編譯錯誤，已刪除舊版重複函式。
- 曾因 `loop()` 保留舊的 `drawChartPage()` 呼叫造成編譯錯誤，已改為 `drawTemperaturePage()`。
- `35_ili_mqtt_ctrl_page` 目前應重新編譯確認四頁版本；尚未在本次交接後完成燒錄驗證。

### 穩定版本備註

- `32_ili9225_gfx_dht` 曾確認顯示正常，是目前 ILI9225 顯示除錯時的穩定參考版本。
- `34_ili_mqtt_ctrl` 為三顆 LED MQTT callback 控制版本。
- 35 版本在 GPIO0 換頁時，開機不要長按按鍵，避免 ESP32 進入燒錄模式。
- 若畫面出現整面雜訊，優先檢查 CS、DC、RST 接線與 SPI 頻率；目前程式使用 30 MHz，必要時可降至 8–15 MHz。

## 十、2026-09-21 電子紙 MQTT 版本最新交接紀錄

### 目前版本與硬體

目前最新版本為 `D:\esp32實習\40_epaper_mqtt\40_epaper_mqtt.ino`，已編譯並上傳至 `COM7`。使用微雪 Waveshare 2.9 吋三色電子紙 V4／Rev2.1，解析度 296×128，ESP32 Dev Module。

電子紙腳位：

- SPI SCK：GPIO18；MOSI／SDI：GPIO23；MISO／SDO 未使用
- CS：GPIO27；DC／RS：GPIO26；RST：GPIO25
- BUSY：GPIO34；PWR：GPIO32（初始化時 HIGH）
- SPI MODE0，2 MHz

驅動檔位於 `D:\esp32實習\40_epaper_mqtt\epd2in9b_V4.cpp`、`epd2in9b_V4.h`、`epdif.cpp`、`epdif.h`。程式以邏輯橫向 296×128 座標繪圖，再轉換為電子紙原生記憶體排列。

### 感測器、LED 與電子紙顯示

- DHT11：GPIO14。
- 光敏電阻：GPIO33，ADC 轉換為 0–100%。
- 綠燈：GPIO15；黃燈：GPIO2；紅燈：GPIO4。
- 顯示 `ENV MONITOR`、溫度／濕度／亮度 ICON、數值與單位。
- DHT11 無效時顯示 `-`。
- 開機完整刷新；之後每 60 秒更新一次。
- 原先嘗試以 `partialRegion(8,284,82,110)` 局部刷新數字區域，但 V4 控制器在大範圍旋轉座標局部刷新時可能長時間維持 BUSY；目前已改為每 60 秒完整刷新黑／紅兩層畫面，以確保實際顯示可靠更新。
- `ReadBusy()` 有 20 秒超時保護，避免 BUSY 異常造成永久卡死。

### Wi-Fi 與 MQTT 設定

- Wi-Fi SSID：`a`
- Wi-Fi 密碼：`12345678`
- Broker：`mqttgo.io`
- Port：`1883`
- Client ID：ESP32 MAC 加隨機值

感測資料發布主題：`eric/class305/data`

```json
{"temp":26,"hum":45,"light":17}
```

DHT 無效時，`temp` 與 `hum` 為 `null`。燈號控制訂閱主題：

- `eric/class305/ctrl/gled`
- `eric/class305/ctrl/yled`
- `eric/class305/ctrl/rled`

控制格式：

```json
{"state":"on"}
{"state":"off"}
```

callback 依主題判斷燈號，再依訊息中的 `on`／`off` 執行 `digitalWrite()`。

### MQTT callback 與 BUSY 處理

電子紙刷新是同步操作；若 `ReadBusy()` 等待時不執行 `mqttClient.loop()`，MQTT callback 會暫停。因此目前在 `40_epaper_mqtt.ino` 加入 `serviceMqttDuringEpd()`，並由 `epd2in9b_V4.cpp` 的 BUSY 等待迴圈呼叫，讓刷新期間仍可處理燈號控制。

### 本次查出的死機原因與修正

曾出現序列埠只停在 `DHT=OK...`，電子紙不更新且 MQTT 停止。加入定位訊息後確認第二次更新卡在 `e-Paper draw done, init start`。原因是每次 `epd.Init()` 都呼叫 `SPI.beginTransaction()`，但原程式沒有結束第一次 SPI transaction，60 秒後第二次刷新卡在 SPI transaction lock。

已在 `Epd::Sleep()` 加入：

```cpp
SPI.endTransaction();
```

修正後已實測第二次更新成功：

```text
e-Paper init done
e-Paper partial transfer start
e-Paper numbers partial updated
e-Paper sleep done
MQTT TX [eric/class305/data] {"temp":26,"hum":46,"light":16}
```

### 重要序列訊息與後續檢查

- `e-Paper full display updated`：開機完整刷新完成。
- `e-Paper periodic full display updated`：60 秒完整刷新完成。
- `MQTT connected and control topics subscribed`：MQTT 已連線並訂閱控制主題。
- `MQTT RX [...]`：收到燈號控制訊息。
- `MQTT TX [...]`：發布感測資料。

若再次卡住，優先確認最後一行序列訊息、BUSY GPIO34、SPI 接線，以及 `SPI.endTransaction()` 是否仍保留。
