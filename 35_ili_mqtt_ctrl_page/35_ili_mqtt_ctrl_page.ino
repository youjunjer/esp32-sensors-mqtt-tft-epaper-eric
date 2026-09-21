#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

const int TFT_SCK=18,TFT_MOSI=23,TFT_CS=27,TFT_DC=26,TFT_RST=25;
const int DHT_PIN=14,LIGHT_PIN=33,PAGE_BUTTON=0;
const int GREEN_LED=15,YELLOW_LED=2,RED_LED=4;
#define SPI_FREQUENCY 30000000UL
const unsigned long UPDATE_INTERVAL=10000UL,RETRY_INTERVAL=5000UL;
const long GMT_OFFSET_SEC=8*3600;
const char* WIFI_SSID="YOUR_WIFI_SSID";const char* WIFI_PASSWORD="YOUR_WIFI_PASSWORD";
const char* MQTT_HOST="mqttgo.io";const int MQTT_PORT=1883;
const char* DATA_TOPIC="eric/class302/data";const char* CTRL_TOPIC="eric/class302/ctrl";

const uint16_t WHITE=0xFFFF,NAVY=0x10A2,PANEL=0x2145,RED=0xF8A6,BLUE=0x45BF,CYAN=0x5DFF,ORANGE=0xFD20,GREEN=0x45A5,YELLOW=0xFFE0,GRAY=0xB596,GRID=0xD69A,LINE_RED=0xA800;
void transfer16(uint16_t v){SPI.transfer(v>>8);SPI.transfer(v);}

class ILI9225_GFX:public Adafruit_GFX{
public:
  ILI9225_GFX():Adafruit_GFX(176,220){}
  void begin(){
    pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);digitalWrite(TFT_CS,HIGH);digitalWrite(TFT_DC,HIGH);SPI.begin(TFT_SCK,-1,TFT_MOSI,TFT_CS);digitalWrite(TFT_RST,LOW);delay(50);digitalWrite(TFT_RST,HIGH);delay(100);
    cmd(0x01,0x011C);cmd(0x02,0x0100);cmd(0x03,0x1030);cmd(0x08,0x0808);cmd(0x0F,0x0801);cmd(0x20,0);cmd(0x21,0);cmd(0x10,0);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);delay(50);cmd(0x10,0x0F00);delay(50);cmd(0x11,0x1B41);cmd(0x12,0x200E);cmd(0x13,0x0D00);cmd(0x14,0x0020);cmd(0x30,0);cmd(0x31,0x00DB);cmd(0x32,0);cmd(0x33,0);cmd(0x34,0x00DB);cmd(0x35,0);cmd(0x36,0x00AF);cmd(0x37,0);cmd(0x38,0x00DB);cmd(0x39,0);cmd(0x07,0x1017);delay(50);
  }
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=0&&y>=0&&x<176&&y<220)fillFast(x,y,1,1,c);}
  void fillFast(int x,int y,int w,int h,uint16_t c){if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>176)w=176-x;if(y+h>220)h=220-y;if(w<=0||h<=0)return;window(x,y,x+w-1,y+h-1);for(long i=0;i<(long)w*h;i++)transfer16(c);digitalWrite(TFT_CS,HIGH);}
private:
  void cmd(uint8_t c,uint16_t d){digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(c);digitalWrite(TFT_DC,HIGH);transfer16(d);digitalWrite(TFT_CS,HIGH);}
  void window(int x0,int y0,int x1,int y1){cmd(0x36,x0);cmd(0x37,x1);cmd(0x38,y0);cmd(0x39,y1);cmd(0x20,x0);cmd(0x21,y0);digitalWrite(TFT_CS,LOW);digitalWrite(TFT_DC,LOW);SPI.transfer(0x22);digitalWrite(TFT_DC,HIGH);}
};

ILI9225_GFX tft;WiFiClient wifiClient;PubSubClient mqtt(wifiClient);SimpleDHT11 dht11(DHT_PIN);
int tempC=0,hum=0,light=0;bool wifiOK=false,mqttOK=false,sensorOK=false,timeOK=false;char timeText[24]="--/-- --- --:--";
unsigned long lastUpdate=0,lastRetry=0,lastButtonChange=0;int page=0;bool lastButton=HIGH;
const int HISTORY_SIZE=60;int temperatureHistory[HISTORY_SIZE],humidityHistory[HISTORY_SIZE],lightHistory[HISTORY_SIZE];int historyCount=0,historyHead=0;

void updateStatusBar(){tft.fillRect(0,0,176,27,WHITE);tft.setTextSize(1);tft.setTextColor(NAVY);tft.setCursor(6,9);tft.print("WIFI: ");tft.print(wifiOK?'O':'X');tft.setCursor(94,9);tft.print("MQTT: ");tft.print(mqttOK?'O':'X');}
void showMessage(const char* title,const char* msg){tft.fillScreen(WHITE);updateStatusBar();tft.setTextColor(NAVY);tft.setTextSize(2);tft.setCursor(12,82);tft.print(title);tft.setTextSize(1);tft.setCursor(12,116);tft.print(msg);}
void thermometer(int x,int y){tft.fillCircle(x,y+18,10,RED);tft.fillRect(x-4,y-15,8,34,RED);tft.fillRect(x-2,y-10,4,28,ORANGE);}
void drop(int x,int y){tft.fillTriangle(x,y-22,x-14,y+7,x+14,y+7,BLUE);tft.fillCircle(x,y+4,14,BLUE);tft.fillCircle(x-4,y-6,2,WHITE);}
void sun(int x,int y){tft.fillCircle(x,y,9,YELLOW);tft.drawCircle(x,y,12,ORANGE);for(int i=0;i<8;i++){float a=i*0.785398f;tft.drawLine(x+(int)(15*cos(a)),y+(int)(15*sin(a)),x+(int)(20*cos(a)),y+(int)(20*sin(a)),ORANGE);}}

void updateClock(){struct tm now;if(getLocalTime(&now,100)){strftime(timeText,sizeof(timeText),"%m/%d %a %H:%M",&now);timeOK=true;}tft.fillRect(0,190,176,30,WHITE);tft.setTextColor(NAVY);tft.setTextSize(1);tft.setCursor(timeOK?39:31,203);tft.print(timeOK?timeText:"TIME --/-- --- --:--");}
void updateValues(){tft.fillRect(50,45,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,51);if(sensorOK)tft.print(tempC);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,58);tft.setTextColor(ORANGE);tft.print("C");tft.fillRect(50,97,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,103);if(sensorOK)tft.print(hum);else tft.print("--");tft.setTextSize(2);tft.setCursor(119,110);tft.setTextColor(CYAN);tft.print("%");tft.fillRect(50,149,80,36,PANEL);tft.setTextColor(WHITE);tft.setTextSize(3);tft.setCursor(55,155);tft.print(light);tft.setTextSize(2);tft.setCursor(119,162);tft.setTextColor(GREEN);tft.print("%");}
void drawSensorPage(){tft.fillScreen(WHITE);updateStatusBar();tft.fillRect(0,27,176,4,CYAN);tft.fillRoundRect(9,33,158,48,6,PANEL);thermometer(31,55);tft.setTextColor(RED);tft.setTextSize(1);tft.setCursor(53,40);tft.print("TEMPERATURE");tft.fillRoundRect(9,85,158,48,6,PANEL);drop(31,107);tft.setTextColor(CYAN);tft.setCursor(53,92);tft.print("HUMIDITY");tft.fillRoundRect(9,137,158,48,6,PANEL);sun(31,159);tft.setTextColor(GREEN);tft.setCursor(53,144);tft.print("LIGHT");updateValues();updateClock();}

void recordTemperature(){
  if(!sensorOK)return;
  temperatureHistory[historyHead]=tempC;humidityHistory[historyHead]=hum;lightHistory[historyHead]=light;
  historyHead=(historyHead+1)%HISTORY_SIZE;if(historyCount<HISTORY_SIZE)historyCount++;
}
int historyAt(int index){int first=(historyHead-historyCount+HISTORY_SIZE)%HISTORY_SIZE;return temperatureHistory[(first+index)%HISTORY_SIZE];}
void drawGaugeDots(float startDeg,float endDeg,uint16_t color){
  const int cx=88,cy=91,radius=43,dotRadius=4,dotCount=11;
  for(int i=0;i<dotCount;i++){
    float ratio=(float)i/(float)(dotCount-1);
    float deg=startDeg+(endDeg-startDeg)*ratio;
    float a=deg*0.0174532925f;
    int x=(int)roundf(cx+radius*cos(a));
    int y=(int)roundf(cy-radius*sin(a));
    tft.fillCircle(x,y,dotRadius,color);
  }
}

void drawMetricGauge(const char* title,int value,int minValue,int maxValue,int boundary1,int boundary2,const char* label0,const char* label1,const char* label2,const char* label3,const char* unit,uint16_t color1,uint16_t color2,uint16_t color3){
  const int cx=88,cy=91;float a1=180.0f-(boundary1-minValue)*180.0f/(maxValue-minValue);float a2=180.0f-(boundary2-minValue)*180.0f/(maxValue-minValue);
  tft.setTextColor(NAVY);tft.setTextSize(1);tft.setCursor(9,34);tft.print(title);
  drawGaugeDots(180,a1,color1);drawGaugeDots(a1,a2,color2);drawGaugeDots(a2,0,color3);tft.fillCircle(cx,cy,32,WHITE);
  tft.setTextColor(NAVY);tft.setTextSize(1);tft.setCursor(28,96);tft.print(label0);tft.setCursor(44,54);tft.print(label1);tft.setCursor(125,54);tft.print(label2);tft.setCursor(136,96);tft.print(label3);
  int clipped=constrain(value,minValue,maxValue);float angle=(180.0f-(clipped-minValue)*180.0f/(maxValue-minValue))*0.0174532925f;int tipX=cx+(int)(34*cos(angle));int tipY=cy-(int)(34*sin(angle));
  tft.drawLine(cx,cy,tipX,tipY,NAVY);tft.fillCircle(cx,cy,7,WHITE);tft.drawCircle(cx,cy,7,NAVY);
  tft.setTextSize(2);tft.setCursor(72,69);if(value>=minValue&&value<=maxValue)tft.print(value);else tft.print("--");tft.setTextSize(1);tft.setCursor(94,75);tft.print(unit);
}

void drawHistoryChart(const char* title,const int* values,int minValue,int maxValue,uint16_t lineColor){
  tft.setTextColor(NAVY);tft.setTextSize(1);tft.setCursor(9,102);tft.print(title);
  const int x0=28,y0=115,x1=169,y1=184;tft.drawRect(x0,y0,x1-x0+1,y1-y0+1,NAVY);
  for(int i=0;i<=3;i++){int value=minValue+(maxValue-minValue)*i/3;int y=y1-i*(y1-y0)/3;tft.drawFastHLine(x0+1,y,x1-x0-1,GRID);tft.setCursor(3,y-3);tft.setTextColor(NAVY);tft.print(value);}
  tft.setCursor(30,191);tft.print("10m ago");tft.setCursor(143,191);tft.print("now");
  if(historyCount==0){tft.setCursor(49,150);tft.print("WAITING DATA");return;}
  for(int i=1;i<historyCount;i++){int prev=constrain(values[(historyHead-historyCount+i-1+HISTORY_SIZE)%HISTORY_SIZE],minValue,maxValue),curr=constrain(values[(historyHead-historyCount+i+HISTORY_SIZE)%HISTORY_SIZE],minValue,maxValue);int px=x0+1+(i-1)*(x1-x0-2)/max(1,historyCount-1);int cx=x0+1+i*(x1-x0-2)/max(1,historyCount-1);int py=y1-1-(prev-minValue)*(y1-y0-2)/(maxValue-minValue);int cy=y1-1-(curr-minValue)*(y1-y0-2)/(maxValue-minValue);tft.drawLine(px,py,cx,cy,lineColor);tft.fillCircle(cx,cy,3,lineColor);}
}

void drawTemperaturePage(){tft.fillScreen(WHITE);updateStatusBar();drawMetricGauge("TEMPERATURE GAUGE",tempC,10,40,20,30,"10","20","30","40","C",GREEN,YELLOW,RED);drawHistoryChart("TEMPERATURE / LAST 10 MINUTES",temperatureHistory,10,40,LINE_RED);}
void drawHumidityPage(){tft.fillScreen(WHITE);updateStatusBar();drawMetricGauge("HUMIDITY GAUGE",hum,0,100,40,70,"0","40","70","100","%",GREEN,YELLOW,RED);drawHistoryChart("HUMIDITY / LAST 10 MINUTES",humidityHistory,0,100,BLUE);}
void drawLightPage(){tft.fillScreen(WHITE);updateStatusBar();drawMetricGauge("LIGHT GAUGE",light,0,100,30,70,"0","30","70","100","%",BLUE,YELLOW,GREEN);drawHistoryChart("LIGHT / LAST 10 MINUTES",lightHistory,0,100,ORANGE);}
void renderPage(){if(page==0)drawSensorPage();else if(page==1)drawTemperaturePage();else if(page==2)drawHumidityPage();else drawLightPage();}

void readSensors(){byte t=0,h=0;sensorOK=(dht11.read(&t,&h,NULL)==SimpleDHTErrSuccess);if(sensorOK){tempC=t;hum=h;}light=constrain(map(analogRead(LIGHT_PIN),0,4095,0,100),0,100);recordTemperature();}
void setLed(JsonVariant v,int pin,const char* name){if(!v.is<const char*>())return;const char* s=v.as<const char*>();if(strcmp(s,"on")==0){digitalWrite(pin,HIGH);Serial.printf("%s ON\n",name);}else if(strcmp(s,"off")==0){digitalWrite(pin,LOW);Serial.printf("%s OFF\n",name);}}
void mqttCallback(char* topic,byte* payload,unsigned int length){if(strcmp(topic,CTRL_TOPIC)!=0)return;StaticJsonDocument<192> doc;DeserializationError e=deserializeJson(doc,payload,length);if(e){Serial.println(e.c_str());return;}setLed(doc["gled"],GREEN_LED,"GREEN");setLed(doc["yled"],YELLOW_LED,"YELLOW");setLed(doc["rled"],RED_LED,"RED");}
void connectWiFi(){wifiOK=false;showMessage("WIFI CONNECT","Connecting...");WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);unsigned long s=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-s<30000)delay(500);wifiOK=WiFi.status()==WL_CONNECTED;showMessage(wifiOK?"WIFI OK":"WIFI ERROR",wifiOK?WiFi.localIP().toString().c_str():"Check SSID/password");delay(800);}
void syncNTP(){configTime(GMT_OFFSET_SEC,0,"pool.ntp.org","time.nist.gov");struct tm now;timeOK=getLocalTime(&now,10000);}
bool connectMQTT(){if(!wifiOK)return false;mqttOK=false;showMessage("MQTT CONNECT","Connecting...");String id="esp32-page-"+String((uint32_t)ESP.getEfuseMac(),HEX)+String(random(0xffff),HEX);mqttOK=mqtt.connect(id.c_str());if(mqttOK){mqtt.subscribe(CTRL_TOPIC);showMessage("MQTT OK",CTRL_TOPIC);delay(600);}return mqttOK;}
void publishData(){if(!mqttOK||!mqtt.connected())return;char payload[80];snprintf(payload,sizeof(payload),"{\"temp\":%d,\"humi\":%d,\"light\":%d}",tempC,hum,light);mqtt.publish(DATA_TOPIC,payload);Serial.println(payload);}

void checkPageButton(){bool now=digitalRead(PAGE_BUTTON);if(lastButton==HIGH&&now==LOW&&millis()-lastButtonChange>250){lastButtonChange=millis();page=(page+1)%4;SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));renderPage();SPI.endTransaction();}lastButton=now;}

void setup(){
  Serial.begin(115200);analogReadResolution(12);pinMode(PAGE_BUTTON,INPUT_PULLUP);pinMode(GREEN_LED,OUTPUT);pinMode(YELLOW_LED,OUTPUT);pinMode(RED_LED,OUTPUT);digitalWrite(GREEN_LED,LOW);digitalWrite(YELLOW_LED,LOW);digitalWrite(RED_LED,LOW);randomSeed((uint32_t)ESP.getEfuseMac());
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));tft.begin();SPI.endTransaction();mqtt.setServer(MQTT_HOST,MQTT_PORT);mqtt.setCallback(mqttCallback);connectWiFi();if(wifiOK)syncNTP();connectMQTT();readSensors();SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));renderPage();SPI.endTransaction();publishData();lastUpdate=millis();
}

void loop(){
  checkPageButton();if(WiFi.status()!=WL_CONNECTED){wifiOK=false;mqttOK=false;connectWiFi();if(wifiOK)syncNTP();}
  if(!mqtt.connected()){mqttOK=false;if(millis()-lastRetry>=RETRY_INTERVAL){lastRetry=millis();if(connectMQTT()){SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));renderPage();SPI.endTransaction();}}}
  mqtt.loop();
  if(millis()-lastUpdate>=UPDATE_INTERVAL){lastUpdate=millis();readSensors();publishData();SPI.beginTransaction(SPISettings(SPI_FREQUENCY,MSBFIRST,SPI_MODE0));if(page==0){updateStatusBar();updateValues();updateClock();}else if(page==1)drawTemperaturePage();else if(page==2)drawHumidityPage();else drawLightPage();SPI.endTransaction();}
  delay(10);
}
