//DHT宣告
#include <SimpleDHT.h>
int pinDHT11 = 23;
SimpleDHT11 dht11(pinDHT11);



void ReadDht(){
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  

  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" H");
}
