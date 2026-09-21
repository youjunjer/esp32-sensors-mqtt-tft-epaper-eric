void setup() {
  // put your setup code here, to run once:
  pinMode(15, OUTPUT);
}

void loop() {
  // 1.5V/3.3V*255=?取整數
  analogWrite(15, 0);  //0V
  delay(1000);
  analogWrite(15, 64);  //64/255*3.3V=>0.82V
  delay(1000);
  analogWrite(15, 128);  //128/255*3.3V=>1.65V
  delay(1000);
  analogWrite(15, 255);  //255/255*3.3V=3.3V
  delay(1000);
  //2.5V      25 0-33 換算 0-255
  int v = map(25, 0, 33, 0, 255);
  analogWrite(15, v);
  delay(1000);
}
