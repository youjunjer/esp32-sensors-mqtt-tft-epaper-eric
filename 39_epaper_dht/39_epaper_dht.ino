#include <Arduino.h>
#include <SPI.h>
#include <SimpleDHT.h>
#include "epd2in9b_V4.h"

constexpr int LOGICAL_WIDTH = 296;
constexpr int LOGICAL_HEIGHT = 128;
constexpr int EPD_BUFFER_BYTES = (EPD_WIDTH / 8) * EPD_HEIGHT;
constexpr int DHT_PIN = 14;
constexpr int LIGHT_PIN = 33;
constexpr unsigned long UPDATE_INTERVAL_MS = 60000UL;

uint8_t blackBuffer[EPD_BUFFER_BYTES];
uint8_t redBuffer[EPD_BUFFER_BYTES];
Epd epd;
SimpleDHT11 dht11(DHT_PIN);
unsigned long lastUpdate = 0;

void glyph(char input, uint8_t out[5]) {
  const char c = (input >= 'a' && input <= 'z') ? input - ('a' - 'A') : input;
  const uint8_t* p = nullptr;
  static const uint8_t blank[5] = {0,0,0,0,0};
  static const uint8_t A[5]={0x7E,0x09,0x09,0x09,0x7E}, C[5]={0x3E,0x41,0x41,0x41,0x22};
  static const uint8_t E[5]={0x7F,0x49,0x49,0x49,0x41}, G[5]={0x3E,0x41,0x49,0x49,0x7A};
  static const uint8_t H[5]={0x7F,0x08,0x08,0x08,0x7F}, I[5]={0x00,0x41,0x7F,0x41,0x00}, O[5]={0x3E,0x41,0x41,0x41,0x3E};
  static const uint8_t L[5]={0x7F,0x40,0x40,0x40,0x40}, M[5]={0x7F,0x02,0x0C,0x02,0x7F};
  static const uint8_t N[5]={0x7F,0x04,0x08,0x10,0x7F}, P[5]={0x7F,0x09,0x09,0x09,0x06};
  static const uint8_t R[5]={0x7F,0x09,0x19,0x29,0x46}, S[5]={0x46,0x49,0x49,0x49,0x31};
  static const uint8_t T[5]={0x01,0x01,0x7F,0x01,0x01}, U[5]={0x3F,0x40,0x40,0x40,0x3F};
  static const uint8_t V[5]={0x1F,0x20,0x40,0x20,0x1F}, W[5]={0x7F,0x20,0x18,0x20,0x7F};
  static const uint8_t Y[5]={0x01,0x02,0x7C,0x02,0x01}, zero[5]={0x3E,0x45,0x49,0x51,0x3E};
  static const uint8_t one[5]={0x00,0x42,0x7F,0x40,0x00}, two[5]={0x62,0x51,0x49,0x49,0x46};
  static const uint8_t three[5]={0x22,0x41,0x49,0x49,0x36}, four[5]={0x18,0x14,0x12,0x7F,0x10};
  static const uint8_t five[5]={0x2F,0x49,0x49,0x49,0x31}, six[5]={0x3E,0x49,0x49,0x49,0x32};
  static const uint8_t seven[5]={0x01,0x71,0x09,0x05,0x03}, eight[5]={0x36,0x49,0x49,0x49,0x36};
  static const uint8_t nine[5]={0x26,0x49,0x49,0x49,0x3E}, dot[5]={0x00,0x60,0x60,0x00,0x00};
  static const uint8_t minus[5]={0x08,0x08,0x08,0x08,0x08}, percent[5]={0x63,0x13,0x08,0x64,0x63};
  switch(c) {
    case 'A':p=A;break;case 'C':p=C;break;case 'E':p=E;break;case 'G':p=G;break;case 'H':p=H;break;
    case 'I':p=I;break;case 'L':p=L;break;case 'M':p=M;break;case 'N':p=N;break;case 'O':p=O;break;case 'P':p=P;break;
    case 'R':p=R;break;case 'S':p=S;break;case 'T':p=T;break;case 'U':p=U;break;case 'V':p=V;break;
    case 'W':p=W;break;case 'Y':p=Y;break;case '0':p=zero;break;case '1':p=one;break;case '2':p=two;break;
    case '3':p=three;break;case '4':p=four;break;case '5':p=five;break;case '6':p=six;break;case '7':p=seven;break;
    case '8':p=eight;break;case '9':p=nine;break;case '.':p=dot;break;case '-':p=minus;break;case '%':p=percent;break;
    default:p=blank;break;
  }
  memcpy(out,p,5);
}

void setNativePixel(uint8_t* buffer, int nativeX, int nativeY, bool colored) {
  if(nativeX<0||nativeX>=EPD_WIDTH||nativeY<0||nativeY>=EPD_HEIGHT)return;
  int index=nativeX+nativeY*EPD_WIDTH; uint8_t mask=0x80>>(index%8);
  if(colored) buffer[index/8]&=static_cast<uint8_t>(~mask); else buffer[index/8]|=mask;
}
void setPixel(uint8_t* buffer,int x,int y,bool colored) {
  if(x<0||x>=LOGICAL_WIDTH||y<0||y>=LOGICAL_HEIGHT)return;
  setNativePixel(buffer,LOGICAL_HEIGHT-1-y,x,colored);
}
void fillRect(uint8_t* b,int x,int y,int w,int h,bool c){for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)setPixel(b,xx,yy,c);}
void line(uint8_t* b,int x0,int y0,int x1,int y1,bool c){int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;while(true){setPixel(b,x0,y0,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}}
void circle(uint8_t* b,int cx,int cy,int r,bool c){for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++)if(x*x+y*y<=r*r)setPixel(b,cx+x,cy+y,c);}
void drawText(const char* text,int x,int y,int scale,bool black,bool red){uint8_t g[5];int cur=x;while(*text){glyph(*text++,g);for(int col=0;col<5;col++)for(int row=0;row<7;row++)if(g[col]&(1<<row))for(int dx=0;dx<scale;dx++)for(int dy=0;dy<scale;dy++){setPixel(blackBuffer,cur+col*scale+dx,y+row*scale+dy,black);setPixel(redBuffer,cur+col*scale+dx,y+row*scale+dy,red);}cur+=6*scale;}}

void drawThermometer(int cx,int cy){
  circle(redBuffer,cx,cy+10,7,true); fillRect(redBuffer,cx-2,cy-15,4,27,true);
  circle(blackBuffer,cx,cy+10,10,true); circle(blackBuffer,cx,cy+10,6,false);
  fillRect(blackBuffer,cx-6,cy-19,12,30,true); fillRect(blackBuffer,cx-2,cy-15,4,27,false);
}
void drawDroplet(int cx,int cy){
  // Classic pointed water-drop icon: black outline, red fill, white highlight.
  circle(blackBuffer,cx,cy+5,15,true);
  for(int y=-20;y<=5;y++){
    int half=(y+20)*15/25;
    for(int x=-half;x<=half;x++) setPixel(blackBuffer,cx+x,cy+y,true);
  }
  circle(redBuffer,cx,cy+5,10,true);
  for(int y=-15;y<=4;y++){
    int half=(y+15)*10/19;
    for(int x=-half;x<=half;x++) setPixel(redBuffer,cx+x,cy+y,true);
  }
  circle(blackBuffer,cx-4,cy-1,3,false);
}
void drawSun(int cx,int cy){
  circle(redBuffer,cx,cy,11,true); circle(blackBuffer,cx,cy,14,true); circle(blackBuffer,cx,cy,9,false);
  for(int i=0;i<8;i++){float a=i*0.7854f;line(blackBuffer,cx+(int)(16*cos(a)),cy+(int)(16*sin(a)),cx+(int)(23*cos(a)),cy+(int)(23*sin(a)),true);}
}

void drawValue(const char* value,int x,int y,const char* unit,int unitX){
  drawText(value,x,y,3,true,false);
  drawText(unit,unitX,y+7,1,false,true);
}

// Copy a logical landscape rectangle from the current black buffer into the
// native portrait byte layout expected by Waveshare Partial().
void partialRegion(int logicalX0,int logicalX1,int logicalY0,int logicalY1){
  int nativeX0 = LOGICAL_HEIGHT - 1 - logicalY1;
  int nativeX1 = LOGICAL_HEIGHT - logicalY0;
  int xByte0 = (nativeX0 / 8) * 8;
  int xByte1 = ((nativeX1 + 7) / 8) * 8;
  int y0 = logicalX0;
  int y1 = logicalX1;
  int rowBytes = (xByte1 - xByte0) / 8;
  int rows = y1 - y0;
  uint8_t region[4 * 90];
  if(rowBytes > 4 || rows > 90) return;
  for(int row=0; row<rows; row++){
    int nativeY = y0 + row;
    memcpy(&region[row * rowBytes], &blackBuffer[(xByte0 / 8) + nativeY * (EPD_WIDTH / 8)], rowBytes);
  }
  epd.Partial(region, xByte0, y0, xByte1, y1);
}

void drawScreen(bool dhtOK, byte temp, byte hum, bool lightOK, int lightPercent){
  memset(blackBuffer,0xFF,sizeof(blackBuffer)); memset(redBuffer,0xFF,sizeof(redBuffer));
  fillRect(redBuffer,0,0,LOGICAL_WIDTH,19,true);
  drawText("ENV MONITOR",78,4,2,false,false);
  fillRect(blackBuffer,98,19,2,109,true); fillRect(blackBuffer,197,19,2,109,true);
  drawThermometer(49,45); drawDroplet(148,45); drawSun(247,45);
  drawText("TEMP",28,66,1,true,false); drawText("HUM",134,66,1,true,false); drawText("LIGHT",219,66,1,true,false);
  if(dhtOK){char t[8],h[8];snprintf(t,sizeof(t),"%d",temp);snprintf(h,sizeof(h),"%d",hum);drawValue(t,17,86,"C",73);drawValue(h,119,86,"%",174);} else {drawValue("-",39,86,"C",73);drawValue("-",140,86,"%",174);}
  if(lightOK){char l[8];snprintf(l,sizeof(l),"%d",lightPercent);drawValue(l,216,86,"%",274);} else drawValue("-",236,86,"%",274);
}

void readAndDisplay(bool fullRefresh){
  byte temperature=0, humidity=0;
  int err=dht11.read(&temperature,&humidity,NULL);
  bool dhtOK=(err==SimpleDHTErrSuccess);
  int raw=analogRead(LIGHT_PIN);
  bool lightOK=(raw>=0);
  int lightPercent=constrain(map(raw,0,4095,0,100),0,100);
  Serial.printf("DHT=%s T=%s H=%s LIGHT=%s %d%%\n",dhtOK?"OK":"FAIL",dhtOK?String(temperature).c_str():"-",dhtOK?String(humidity).c_str():"-",lightOK?"OK":"FAIL",lightPercent);
  drawScreen(dhtOK,temperature,humidity,lightOK,lightPercent);
  if(epd.Init()!=0){Serial.println("e-Paper init failed");return;}
  if(fullRefresh){
    epd.Display(blackBuffer,redBuffer);
    Serial.println("e-Paper full display updated");
  }else{
    partialRegion(8,94,82,110);    // temperature number
    partialRegion(108,192,82,110);  // humidity number
    partialRegion(205,284,82,110);   // brightness number
    Serial.println("e-Paper numbers partial updated");
  }
  epd.Sleep();
}

void setup(){Serial.begin(115200);delay(300);analogReadResolution(12);pinMode(LIGHT_PIN,INPUT);Serial.println("39_epaper_dht");readAndDisplay(true);lastUpdate=millis();}
void loop(){if(millis()-lastUpdate>=UPDATE_INTERVAL_MS){lastUpdate=millis();readAndDisplay(false);}delay(100);}
