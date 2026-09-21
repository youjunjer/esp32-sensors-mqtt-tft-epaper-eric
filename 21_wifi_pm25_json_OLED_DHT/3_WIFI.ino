#include <WiFi.h>         //WiFi
#include <HTTPClient.h>   //瀏覽器
#include <ArduinoJson.h>  //請先安裝ArduinoJson程式庫

char ssid[] = "YOUR_WIFI_SSID";                                                                                   //請修改為您連線的網路名稱
char password[] = "YOUR_WIFI_PASSWORD";                                                                           //請修改為您連線的網路密碼
char url[] = "https://data.moenv.gov.tw/api/v2/aqx_p_02?api_key=YOUR_API_KEY&limit=100";                         //PM2.5的網址

void wifiConnect() {
  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);
  //1.設定WiFi模式
  WiFi.mode(WIFI_STA);
  //2.啟動WiFi連線
  WiFi.begin(ssid, password);
  //3.檢查連線狀態
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("連線完成");
}

void readPM25() {
  Serial.println("啟動網頁連線");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("httpCode=");
  Serial.println(httpCode);
  //5.檢查網頁連線是否正常
  if (httpCode == HTTP_CODE_OK) {
    //6.取得網頁內容
    String payload = http.getString();
    //Serial.print("payload=");
    //7.將資料顯示在螢幕上
    //Serial.println(payload);
    //JSON格式解析
    DynamicJsonDocument AQIJson(payload.length() * 2);  //宣告一個JSON文件，名稱為AQIJson
    deserializeJson(AQIJson, payload);                  //解析網頁內容payload為JSON格式，存放在AQIJson內
    for (int i = 0; i < AQIJson["records"].size(); i++) {
      // 瀏覽records內的所有紀錄，直到找到site=="橋頭"
      if (AQIJson["records"][i]["site"] == "前鎮") {
        String temp = AQIJson["records"][i]["pm25"];
        AQI = temp;
        Serial.println("前鎮 PM2.5=" + AQI);
        break;
      }
    }
  }
  http.end();
}
