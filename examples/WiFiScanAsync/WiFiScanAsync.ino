/*
    This sketch demonstrates how to scan wifi networks.
    The API is almost the same as with the wifi Shield library,
    the most obvious difference being the different file you need to include:
*/
#include <teensquitto.h>
teensquitto wifi = teensquitto("WiFi", &Serial1);

void setup() {
  Serial.begin(115200);
  Serial1.begin(2000000);

  // Set wifi to station mode and disconnect from an AP if it was previously connected
  wifi.mode(WIFI_STA);
  wifi.disconnect();
  delay(100);

  Serial.println("Setup done");
}

void loop() {
  Serial.println("scan start");
  String json = "[";
  // wifi.scanNetworks will return the number of networks found
  int n = wifi.scanComplete();
  if(n == -2){
    wifi.scanNetworks(true);
    Serial.println("No networks found");
  } else if(n){
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      json += "{";
      json += " \"rssi\":"+String(wifi.RSSI(i));
      json += ",  \"ssid\":\""+wifi.SSID(i)+"\"";
      json += ",  \"bssid\":\""+wifi.BSSIDstr(i)+"\"";
      json += ",  \"channel\":"+String(wifi.channel(i));
      json += ",  \"secure\":"+String(wifi.encryptionType(i));
      json += ",  \"hidden\":"+String(wifi.isHidden(i)?"true":"false");
      json += " }";
      Serial.println(json);
      json = String();
    }
    wifi.scanDelete();
    delay(2500);
    if(wifi.scanComplete() == -2){
      wifi.scanNetworks(true);
    }
  }
  Serial.println("=====================\n");
  json = String();
  // Wait a bit before scanning again
  wifi.events();
}

void serialEvent1(){
  wifi.events();
}

