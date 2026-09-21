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
  digitalWrite(GreenPin, HIGH);
  delay(3000);
  digitalWrite(redPin, HIGH);//黃=綠+紅
  delay(1000);
  digitalWrite(GreenPin, LOW);
  delay(5000);
  digitalWrite(redPin, LOW);
}
