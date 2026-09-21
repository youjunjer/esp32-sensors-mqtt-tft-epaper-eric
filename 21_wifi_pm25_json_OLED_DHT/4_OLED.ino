//OLED宣告
#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);


//void 無回傳值
void OLEDBegin(){
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始
}

void OLEDShow(String Line1,String Line2,String Line3){
  //OLED顯示數據
  u8g2.clearBuffer();                        //顯示前清除螢幕

  u8g2.setCursor(0, 5);                     //移動游標
  u8g2.print(Line1);           //寫入文字

  u8g2.setCursor(0, 25);                     //移動游標
  u8g2.print(Line2);           //寫入文字

  u8g2.setCursor(0, 45);                    //移動游標
  u8g2.print(Line3);  //寫入文字

  u8g2.sendBuffer();  //送到螢幕顯示
}
