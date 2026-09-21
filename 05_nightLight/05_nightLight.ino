void setup() {
  // put your setup code here, to run once:
  pinMode(36, INPUT);
  pinMode(15, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(2, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  int light = analogRead(36);
  //新  等比調整 舊  小 大  新小 新大
  light = map(light, 0, 4095, 100, 0);
  if (light > 60) {
    digitalWrite(15, LOW);
    digitalWrite(4, LOW);
    digitalWrite(2, LOW);
  } else if (light > 40) {
    digitalWrite(15, HIGH);
    digitalWrite(4, LOW);
    digitalWrite(2, LOW);
  } else if (light > 20) {
    digitalWrite(15, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(2, LOW);
  } else {
    digitalWrite(15, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(2, HIGH);
  }
  delay(100);
}
