void setup() {
  // put your setup code here, to run once:
  pinMode(15, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(15, HIGH);
  delay(5000);
  digitalWrite(15,LOW);
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  digitalWrite(4, HIGH);
  delay(3000);
  digitalWrite(4, LOW);
}