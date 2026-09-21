#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

const int TFT_SCK=18, TFT_MOSI=23, TFT_CS=27, TFT_DC=26, TFT_RST=25;
const int DHT_PIN=14, LIGHT_PIN=33;
#define SPI_FREQUENCY 30000000UL
const unsigned long MQTT_INTERVAL=10000UL;
const unsigned long MQTT_RETRY_INTERVAL=5000UL;

const char* WIFI_SSID="YOUR_WIFI_SSID";
const char* WIFI_PASSWORD="YOUR_WIFI_PASSWORD";
const char* MQTT_HOST="mqttgo.io";
const int MQTT_PORT=1883;
const char* MQTT_TOPIC="eric/class302/data";

const uint16_t WHITE=0xFFFF, NAVY=0x10A2, PANEL=0x2145, RED=0xF8A6;
const uint16_t BLUE=0x45BF, CYAN=0x5DFF, ORANGE=0xFD20, GREEN=0x45A5, YELLOW=0xFFE0;

void transfer16(uint16_t value){SPI.transfer(value>>8);SPI.transfer(value);}

class ILI9225_GFX:public Adafruit_GFX{
public:
  ILI9225_GFX():Adafruit_GFX(176,220){}
  void begin(){
    pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);
    digitalWrite(TFT_CS,HIGH);digitalWrite(TFT_DC,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);
    digitalWrite(TFT_RST,LOW);delay(50);digitalWrite(TFT_RST,HIGH);delay(100);
    cmd(0x01,0x011C);cmd(0x02,0x0100);cmd(0x03,0x1030);cmd(0x08,0x0808);cmd(0x0F,0x0801);
    cmd(0x20,0);cmd(0x21,0);cmd(0x10,0);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);
    delay(50);cmd(0x10,0x0F00);delay(50);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);
    cmd(0x30,0);cmd(0x31,0x00DB);cmd(0x32,0);cmd(0x33,0);cmd(0x34,0x00DB);cmd(0x35,0);cmd(0x36,0x00AF);cmd(0x37,0);cmd(0x38,0x00DB);cmd(0x39,0);cmd(0x07,0x1017);delay(50);
  }
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=0&&y>=0&&x<176&&y<220)fillFast(x,y,1,1,c);}
  void fillFast(int x,int y,int w,int h,uint16_t c){
    if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>176)w=176-x;if(y+h>220)h=220-y;if(w<=0||h<=0)return;
    window(x,y,x+w-1,y+h-1);for(long i=0;i<(long)w*h;i++)transfer16(c);digitalWrite(TFT_CS,HIGH);
  }
private:
  void cmd(uint8_t c,uint16_t d){digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(c);digitalWrite(TFT_DC,HIGH);transfer16(d);digitalWrite(TFT_CS,HIGH);}
  void window(int x0,int y0,int x1,int y1){cmd(0x36,x0);cmd(0x37,x1);cmd(0x38,y0);cmd(0x39,y1);cmd(0x20,x0);cmd(0x21,y0);digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(0x22);digitalWrite(TFT_DC,HIGH);}
};

ILI9225_GFX tft; WiFiClient wifiClient; PubSubClient mqtt(wifiClient); SimpleDHT11 dht11(DHT_PIN);
int tempC=0,hum=0,light=0; bool wifiOK=false,mqttOK=false,sensorOK=false;
unsigned long lastPublish=0,lastMqttRetry=0;

void updateStatusBar(){
  tft.fillRect(0,0,176,27,WHITE);tft.setTextSize(1);tft.setTextColor(NAVY);tft.setCursor(6,9);tft.print("WIFI: ");tft.print(wifiOK?'O':'X');
  tft.setCursor(94,9);tft.print("MQTT: ");tft.print(mqttOK?'O':'X');
}

void showMessage(const char* title,const char* message){
  tft.fillScreen(WHITE);updateStatusBar();tft.setTextColor(NAVY);tft.setTextSize(2);tft.setCursor(12,82);tft.print(title);
  tft.setTextSize(1);tft.setCursor(12,116);tft.print(message);
}

void drawThermometer(int x,int y){tft.fillCircle(x,y+18,10,RED);tft.fillRect(x-4,y-15,8,34,RED);tft.fillRect(x-2,y-10,4,28,ORANGE);}
void drawDrop(int x,int y){tft.fillTriangle(x,y-22,x-14,y+7,x+14,y+7,BLUE);tft.fillCircle(x,y+4,14,BLUE);tft.fillCircle(x-4,y-6,2,WHITE);}
void drawSun(int x,int y){tft.fillCircle(x,y,9,YELLOW);tft.drawCircle(x,y,12,ORANGE);for(int i=0;i<8;i++){float a=i*0.785398f;tft.drawLine(x+(int)(15*cos(a)),y+(int)(15*sin(a)),x+(int)(20*cos(a)),y+(int)(20*sin(a)),ORANGE);}}

void drawStaticScreen(){
  tft.fillScreen(WHITE);updateStatusBar();tft.fillRect(0,27,176,4,CYAN);
  tft.fillRoundRect(9,35,158,53,6,PANEL);drawThermometer(31,59);tft.setTextColor(RED);tft.setTextSize(1);tft.setCursor(53,44);tft.print("TEMPERATURE");
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,57);if(sensorOK)tft.print(tempC);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,64);tft.setTextColor(ORANGE);tft.print("C");
  tft.fillRoundRect(9,95,158,53,6,PANEL);drawDrop(31,119);tft.setTextColor(CYAN);tft.setTextSize(1);tft.setCursor(53,104);tft.print("HUMIDITY");
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,117);if(sensorOK)tft.print(hum);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,124);tft.setTextColor(CYAN);tft.print("%");
  tft.fillRoundRect(9,155,158,53,6,PANEL);drawSun(31,179);tft.setTextColor(GREEN);tft.setTextSize(1);tft.setCursor(53,164);tft.print("LIGHT");
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,177);tft.print(light);tft.setTextSize(2);tft.setCursor(119,184);tft.setTextColor(GREEN);tft.print("%");
}

// 只更新溫度、濕度、亮度數值與單位，不重畫背景、卡片和圖示
void updateValues(){
  tft.fillRect(50,51,80,38,PANEL);
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,57);if(sensorOK)tft.print(tempC);else tft.print("--");
  tft.setTextSize(2);tft.setCursor(119,64);tft.setTextColor(ORANGE);tft.print("C");

  tft.fillRect(50,111,80,38,PANEL);
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,117);if(sensorOK)tft.print(hum);else tft.print("--");
  tft.setTextSize(2);tft.setCursor(119,124);tft.setTextColor(CYAN);tft.print("%");

  tft.fillRect(50,171,80,38,PANEL);
  tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,177);tft.print(light);
  tft.setTextSize(2);tft.setCursor(119,184);tft.setTextColor(GREEN);tft.print("%");
}

void readSensors(){byte t=0,h=0;sensorOK=(dht11.read(&t,&h,NULL)==SimpleDHTErrSuccess);if(sensorOK){tempC=t;hum=h;}light=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);}

void connectWiFi(){
  wifiOK=false;showMessage("WIFI CONNECT","Connecting...");WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  unsigned long start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<30000){delay(500);}
  wifiOK=(WiFi.status()==WL_CONNECTED);showMessage(wifiOK?"WIFI OK":"WIFI ERROR",wifiOK?WiFi.localIP().toString().c_str():"Check SSID/password");delay(1200);
}

bool connectMQTT(){
  if(!wifiOK)return false;mqttOK=false;showMessage("MQTT CONNECT","Connecting...");
  String id="esp32-ili-"+String((uint32_t)ESP.getEfuseMac(),HEX)+String(random(0xffff),HEX);
  mqttOK=mqtt.connect(id.c_str());if(mqttOK){showMessage("MQTT OK",MQTT_TOPIC);delay(800);}return mqttOK;
}

void publishData(){
  if(!mqttOK||!mqtt.connected())return;char payload[80];snprintf(payload,sizeof(payload),"{\"temp\":%d,\"humi\":%d,\"light\":%d}",tempC,hum,light);mqtt.publish(MQTT_TOPIC,payload);Serial.println(payload);
}

void setup(){
  Serial.begin(115200);analogReadResolution(12);randomSeed((uint32_t)ESP.getEfuseMac());
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));tft.begin();SPI.endTransaction();
  connectWiFi();mqtt.setServer(MQTT_HOST,MQTT_PORT);connectMQTT();readSensors();
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));drawStaticScreen();SPI.endTransaction();publishData();lastPublish=millis();
}

void loop(){
  if(WiFi.status()!=WL_CONNECTED){wifiOK=false;mqttOK=false;connectWiFi();}
  if(!mqtt.connected()){
    mqttOK=false;
    if(millis()-lastMqttRetry>=MQTT_RETRY_INTERVAL){
      lastMqttRetry=millis();
      if(connectMQTT()){
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));drawStaticScreen();SPI.endTransaction();
      }
    }
  }
  mqtt.loop();
  if(millis()-lastPublish>=MQTT_INTERVAL){
    lastPublish=millis();readSensors();publishData();
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));
    updateStatusBar();updateValues();
    SPI.endTransaction();
  }
  delay(10);
}
