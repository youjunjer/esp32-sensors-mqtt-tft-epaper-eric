String AQI="";
byte temperature = 0;
byte humidity = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  OLEDBegin();
  wifiConnect();
}

void loop() {
  readPM25();
  ReadDht();
  OLEDShow("   PM25：" + AQI + " ug","   溫度：" + String(temperature) + " C","   濕度：" + String(humidity) + " %");
  delay(30000);
}