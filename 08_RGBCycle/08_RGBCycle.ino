int redPin = 4;
int greenPin = 2;
int bluePin = 15;
void setup() {
  // put your setup code here, to run once:
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  analogWrite(redPin, 0);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 0);
  delay(100);
}

void loop() {
  //綠慢慢變紅=>紅慢慢大,綠慢慢小
  for (int i = 0; i <= 255; i++) {
    analogWrite(redPin, i);          //紅燈慢慢亮
    analogWrite(greenPin, 255 - i);  //綠燈慢慢暗
    delay(20);
  }
  //紅慢慢變藍=>紅慢慢小,藍慢慢大
  for (int i = 0; i <= 255; i++) {
    analogWrite(bluePin, i);       //藍燈慢慢亮
    analogWrite(redPin, 255 - i);  //紅燈慢慢暗
    delay(20);
  }
  //藍慢慢變綠=>藍慢慢小,綠慢慢大
  for (int i = 0; i <= 255; i++) {
    analogWrite(greenPin, i);       //綠燈慢慢亮
    analogWrite(bluePin, 255 - i);  //藍燈慢慢暗
    delay(20);
  }
  delay(1000);
}
