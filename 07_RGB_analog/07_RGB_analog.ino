int redPin = 4;
int GreenPin = 2;
int bluePin = 15;
void setup() {
  // put your setup code here, to run once:
  pinMode(redPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}
void loop() {
  //用類比顯示紅綠燈
  analogWrite(GreenPin, 255);
  delay(3000);
  analogWrite(redPin, 255);
  delay(1000);
  analogWrite(GreenPin, 0);
  delay(5000);
  //淡紫 f05dc5=>240,93,197
  analogWrite(redPin, 240);
  analogWrite(GreenPin, 93);
  analogWrite(bluePin, 197);
  delay(3000);
  //關閉紅,藍
  analogWrite(redPin, 0);
  analogWrite(bluePin, 0);
}







