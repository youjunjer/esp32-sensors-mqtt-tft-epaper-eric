#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

const int TFT_SCK=18,TFT_MOSI=23,TFT_CS=27,TFT_DC=26,TFT_RST=25;
const int DHT_PIN=14,LIGHT_PIN=33;
const int GREEN_LED=15,YELLOW_LED=2,RED_LED=4;
#define SPI_FREQUENCY 30000000UL
const unsigned long MQTT_INTERVAL=10000UL, MQTT_RETRY_INTERVAL=5000UL;
const long GMT_OFFSET_SEC=8*3600;
const char* WIFI_SSID="YOUR_WIFI_SSID";const char* WIFI_PASSWORD="YOUR_WIFI_PASSWORD";
const char* MQTT_HOST="mqttgo.io";const int MQTT_PORT=1883;
const char* MQTT_DATA_TOPIC="eric/class302/data";const char* MQTT_CTRL_TOPIC="eric/class302/ctrl";

const uint16_t WHITE=0xFFFF,NAVY=0x10A2,PANEL=0x2145,RED=0xF8A6,BLUE=0x45BF,CYAN=0x5DFF,ORANGE=0xFD20,GREEN=0x45A5,YELLOW=0xFFE0;
void transfer16(uint16_t v){SPI.transfer(v>>8);SPI.transfer(v);}

class ILI9225_GFX:public Adafruit_GFX{
public:
  ILI9225_GFX():Adafruit_GFX(176,220){}
  void begin(){
    pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_CS,HIGH);digitalWrite(TFT_DC,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);
    digitalWrite(TFT_RST,LOW);delay(50);digitalWrite(TFT_RST,HIGH);delay(100);
    cmd(0x01,0x011C);cmd(0x02,0x0100);cmd(0x03,0x1030);cmd(0x08,0x0808);cmd(0x0F,0x0801);cmd(0x20,0);cmd(0x21,0);cmd(0x10,0);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);delay(50);cmd(0x10,0x0F00);delay(50);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);cmd(0x30,0);cmd(0x31,0x00DB);cmd(0x32,0);cmd(0x33,0);cmd(0x34,0x00DB);cmd(0x35,0);cmd(0x36,0x00AF);cmd(0x37,0);cmd(0x38,0x00DB);cmd(0x39,0);cmd(0x07,0x1017);delay(50);
  }
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=0&&y>=0&&x<176&&y<220)fillFast(x,y,1,1,c);}
  void fillFast(int x,int y,int w,int h,uint16_t c){if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>176)w=176-x;if(y+h>220)h=220-y;if(w<=0||h<=0)return;window(x,y,x+w-1,y+h-1);for(long i=0;i<(long)w*h;i++)transfer16(c);digitalWrite(TFT_CS,HIGH);}
private:
  void cmd(uint8_t c,uint16_t d){digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(c);digitalWrite(TFT_DC,HIGH);transfer16(d);digitalWrite(TFT_CS,HIGH);}
  void window(int x0,int y0,int x1,int y1){cmd(0x36,x0);cmd(0x37,x1);cmd(0x38,y0);cmd(0x39,y1);cmd(0x20,x0);cmd(0x21,y0);digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(0x22);digitalWrite(TFT_DC,HIGH);}
};

ILI9225_GFX tft;WiFiClient wifiClient;PubSubClient mqtt(wifiClient);SimpleDHT11 dht11(DHT_PIN);
int tempC=0,hum=0,light=0;bool wifiOK=false,mqttOK=false,sensorOK=false;unsigned long lastPublish=0,lastMqttRetry=0;
char timeText[24]="TIME --/-- --- --:--";bool timeOK=false;

void updateStatusBar(){tft.fillRect(0,0,176,27,WHITE);tft.setTextSize(1);tft.setTextColor(NAVY);tft.setCursor(6,9);tft.print("WIFI: ");tft.print(wifiOK?'O':'X');tft.setCursor(94,9);tft.print("MQTT: ");tft.print(mqttOK?'O':'X');}
void showMessage(const char* title,const char* message){tft.fillScreen(WHITE);updateStatusBar();tft.setTextColor(NAVY);tft.setTextSize(2);tft.setCursor(12,82);tft.print(title);tft.setTextSize(1);tft.setCursor(12,116);tft.print(message);}
void drawThermometer(int x,int y){tft.fillCircle(x,y+18,10,RED);tft.fillRect(x-4,y-15,8,34,RED);tft.fillRect(x-2,y-10,4,28,ORANGE);}
void drawDrop(int x,int y){tft.fillTriangle(x,y-22,x-14,y+7,x+14,y+7,BLUE);tft.fillCircle(x,y+4,14,BLUE);tft.fillCircle(x-4,y-6,2,WHITE);}
void drawSun(int x,int y){tft.fillCircle(x,y,9,YELLOW);tft.drawCircle(x,y,12,ORANGE);for(int i=0;i<8;i++){float a=i*0.785398f;tft.drawLine(x+(int)(15*cos(a)),y+(int)(15*sin(a)),x+(int)(20*cos(a)),y+(int)(20*sin(a)),ORANGE);}}
void updateClock(){
  struct tm now;
  if(getLocalTime(&now,100)){strftime(timeText,sizeof(timeText),"%m/%d %a %H:%M",&now);timeOK=true;}
  tft.fillRect(0,190,176,30,WHITE);tft.setTextColor(NAVY);tft.setTextSize(1);tft.setCursor(timeOK?39:31,203);tft.print(timeOK?timeText:"TIME --/-- --- --:--");
}

void drawStaticScreen(){
  tft.fillScreen(WHITE);updateStatusBar();tft.fillRect(0,27,176,4,CYAN);
  tft.fillRoundRect(9,33,158,48,6,PANEL);drawThermometer(31,55);tft.setTextColor(RED);tft.setTextSize(1);tft.setCursor(53,40);tft.print("TEMPERATURE");
  tft.fillRoundRect(9,85,158,48,6,PANEL);drawDrop(31,107);tft.setTextColor(CYAN);tft.setTextSize(1);tft.setCursor(53,92);tft.print("HUMIDITY");
  tft.fillRoundRect(9,137,158,48,6,PANEL);drawSun(31,159);tft.setTextColor(GREEN);tft.setTextSize(1);tft.setCursor(53,144);tft.print("LIGHT");
  updateValues();
  updateClock();
}

void updateValues(){
  tft.fillRect(50,45,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,51);if(sensorOK)tft.print(tempC);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,58);tft.setTextColor(ORANGE);tft.print("C");
  tft.fillRect(50,97,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,103);if(sensorOK)tft.print(hum);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,110);tft.setTextColor(CYAN);tft.print("%");
  tft.fillRect(50,149,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,155);tft.print(light);tft.setTextSize(2);tft.setCursor(119,162);tft.setTextColor(GREEN);tft.print("%");
}

void readSensors(){byte t=0,h=0;sensorOK=(dht11.read(&t,&h,NULL)==SimpleDHTErrSuccess);if(sensorOK){tempC=t;hum=h;}light=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);}

void setLedFromJson(JsonVariant value,int pin,const char* name){if(value.is<const char*>()){const char* state=value.as<const char*>();if(strcmp(state,"on")==0){digitalWrite(pin,HIGH);Serial.printf("%s ON\n",name);}else if(strcmp(state,"off")==0){digitalWrite(pin,LOW);Serial.printf("%s OFF\n",name);}}}

void mqttCallback(char* topic,byte* payload,unsigned int length){
  if(strcmp(topic,MQTT_CTRL_TOPIC)!=0)return;
  StaticJsonDocument<192> doc;DeserializationError error=deserializeJson(doc,payload,length);
  if(error){Serial.print("JSON error: ");Serial.println(error.c_str());return;}
  setLedFromJson(doc["gled"],GREEN_LED,"GREEN");setLedFromJson(doc["yled"],YELLOW_LED,"YELLOW");setLedFromJson(doc["rled"],RED_LED,"RED");
}

void connectWiFi(){wifiOK=false;showMessage("WIFI CONNECT","Connecting...");WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);unsigned long start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<30000){delay(500);}wifiOK=(WiFi.status()==WL_CONNECTED);showMessage(wifiOK?"WIFI OK":"WIFI ERROR",wifiOK?WiFi.localIP().toString().c_str():"Check SSID/password");delay(1000);}
void syncNTP(){configTime(GMT_OFFSET_SEC,0,"pool.ntp.org","time.nist.gov");struct tm now;timeOK=getLocalTime(&now,10000);}
bool connectMQTT(){if(!wifiOK)return false;mqttOK=false;showMessage("MQTT CONNECT","Connecting...");String id="esp32-ctrl-"+String((uint32_t)ESP.getEfuseMac(),HEX)+String(random(0xffff),HEX);mqttOK=mqtt.connect(id.c_str());if(mqttOK){mqtt.subscribe(MQTT_CTRL_TOPIC);showMessage("MQTT OK",MQTT_CTRL_TOPIC);delay(700);}return mqttOK;}
void publishData(){if(!mqttOK||!mqtt.connected())return;char payload[80];snprintf(payload,sizeof(payload),"{\"temp\":%d,\"humi\":%d,\"light\":%d}",tempC,hum,light);mqtt.publish(MQTT_DATA_TOPIC,payload);Serial.println(payload);}

void setup(){
  Serial.begin(115200);analogReadResolution(12);randomSeed((uint32_t)ESP.getEfuseMac());
  pinMode(GREEN_LED,OUTPUT);pinMode(YELLOW_LED,OUTPUT);pinMode(RED_LED,OUTPUT);digitalWrite(GREEN_LED,LOW);digitalWrite(YELLOW_LED,LOW);digitalWrite(RED_LED,LOW);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));tft.begin();SPI.endTransaction();
  mqtt.setServer(MQTT_HOST,MQTT_PORT);mqtt.setCallback(mqttCallback);connectWiFi();if(wifiOK)syncNTP();connectMQTT();readSensors();
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));drawStaticScreen();SPI.endTransaction();publishData();lastPublish=millis();
}

void loop(){
  if(WiFi.status()!=WL_CONNECTED){wifiOK=false;mqttOK=false;connectWiFi();if(wifiOK)syncNTP();}
  if(!mqtt.connected()){mqttOK=false;if(millis()-lastMqttRetry>=MQTT_RETRY_INTERVAL){lastMqttRetry=millis();if(connectMQTT()){SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));drawStaticScreen();SPI.endTransaction();}}}
  mqtt.loop();
  if(millis()-lastPublish>=MQTT_INTERVAL){lastPublish=millis();readSensors();publishData();SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));updateStatusBar();updateValues();updateClock();SPI.endTransaction();}
  delay(10);
}
