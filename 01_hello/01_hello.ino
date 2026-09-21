void setup() {
  //程式開始，執行一次
  Serial.begin(115200);  //序列啟動,提供除錯訊息,115200速度
}

void loop() {
  //重複執行,無止無盡
  Serial.println("哈樓你好~我是大美女!");
  delay(1000);  //1000ms=1秒
}
