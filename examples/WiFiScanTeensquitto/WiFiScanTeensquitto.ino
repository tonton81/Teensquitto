/*
    This sketch demonstrates how to scan wifi networks.
    The API is almost the same as with the wifi Shield library,
    the most obvious difference being the different file you need to include:
*/
#include <teensquitto.h>
teensquitto wifi = teensquitto("WiFi", &Serial1);

void setup() {
  Serial.begin(1);
  Serial2.begin(2000000);
  Serial1.begin(2000000); 
  Serial.println("Running");
  delay(2000);
  // Set wifi to station mode and disconnect from an AP if it was previously connected
  ESP.onDetect([]() {
    wifi.mode(WIFI_STA);
    wifi.disconnect();
    Serial.println("Setup done");
  });
}

void loop() {
  Serial.println("======================\n");
  Serial.println("scan start");

  // wifi.scanNetworks will return the number of networks found
  int n = wifi.scanNetworks();
  Serial.println("scan done using scanNetworks:");
  WiFi.events();
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(wifi.SSID(i));
      Serial.print(" (");
      Serial.print(wifi.RSSI(i));
      Serial.print(")");
      Serial.println((wifi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }

  Serial.println("-----------------------\n");
  //wifi.scanNetworksAsync(hndl, 1);
  
  wifi.events();
  
  // Wait a bit before scanning again
  delay(500);
  wifi.events();
}

void serialEvent1() {
 //if ( Serial1.available() ){
  //String str1;
  //str1 = Serial1.readString();
  //Serial.println("Serial1 Event: "); //Serial.println(str1);  
  //Serial.write(Serial1.read());
  //}
  wifi.events();
}

void serialEvent2() {
  wifi.events();
 if ( Serial2.available() ){
  String str1;
  str1 = Serial2.readString();
  Serial.println("Serial2 Event: "); Serial.println(str1);  
  }
}

void hndl(int networksFound) {
  Serial.print("Async Networks Found: "); Serial.println(networksFound);
    Serial.println(" networks found");
    for (int i = 0; i < networksFound; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(wifi.SSID(i));
      Serial.print(" (");
      Serial.print(wifi.RSSI(i));
      Serial.print(")");
      Serial.println((wifi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }

