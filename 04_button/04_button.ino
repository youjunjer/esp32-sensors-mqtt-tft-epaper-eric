void setup() {
  // put your setup code here, to run once:
  pinMode(16, INPUT);
  pinMode(15, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(digitalRead(16));
  if (digitalRead(16) == 1) digitalWrite(15, HIGH);
  else digitalWrite(15, LOW);
  delay(100);
}
