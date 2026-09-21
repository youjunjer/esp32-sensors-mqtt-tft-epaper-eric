void setup() {
  // put your setup code here, to run once:
  pinMode(15, OUTPUT);    //15腳為輸出用,接LED
  digitalWrite(15, LOW);  //對15輸出高電壓(3.3V)
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(15, HIGH);
  delay(10);
  digitalWrite(15, LOW);
  delay(10);
}
