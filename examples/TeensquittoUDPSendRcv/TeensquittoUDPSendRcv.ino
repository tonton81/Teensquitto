#include <teensquitto.h>
#include <IPAddress.h>
#include <string>

teensquitto UdpS = teensquitto("WiFiUdp", &Serial1);
teensquitto wifi = teensquitto("WiFi", &Serial1);

    IPAddress ip(192, 168, 1, 162);
    IPAddress gate(192, 168, 1, 1);
    IPAddress sub(255, 255, 255, 0);
    const char* ssid = "..................";
    const char* password = "...............";

    //WiFiUDP Udp;
    unsigned int localUdpPort = 4210;  // local port to listen on
    unsigned int localUdpPortS = 3333;  // local port to listen on
    char incomingPacket[255];  // buffer for incoming packets
    char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back
    char  replyPacekt1[] = "Loop de Loop :-)";

    void setup()
    {
      Serial.begin(115200);
      Serial1.begin(2000000);
      //Serial2.begin(2000000);
      Serial.println();
      Serial.printf("Connecting to %s ", ssid);


  ESP.onDetect([]() {
    wifi.mode(WIFI_STA);
    wifi.begin(ssid, password);
    wifi.waitForConnectResult();
    wifi.config(ip, gate, sub);
    if (udp.listen(localUdpPort) ) {
      udp.onPacket([](AsyncUDPPacket packet) {
        Serial.print("UDP Packet Type: ");
        Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
        Serial.print(", From: ");
        Serial.print(packet.remoteIP());
        Serial.print(":");
        Serial.print(packet.remotePort());
        Serial.print(", To: ");
        Serial.print(packet.localIP());
        Serial.print(":");
        Serial.print(packet.localPort());
        Serial.print(", Length: ");
        Serial.print(packet.length());
        Serial.print(", Data: ");
        Serial.write(packet.data(), packet.length());
        Serial.println();
        // reply to the client
        packet.printf("Got %u bytes of data", packet.length());
      });
    }
  }); // end of onDetect callback
  
  UdpS.begin(localUdpPortS);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPortS);

}

void loop()
{
      
    UdpS.beginPacket(UdpS.remoteIP(), UdpS.remotePort());
    UdpS.print(replyPacekt1);
    UdpS.endPacket();
    wifi.events();
    delay(50);
      
     wifi.events();
}

void serialEvent1() {
  wifi.events();
}

//void serialEvent2() {
// if ( Serial2.available() ){
//  String str1;
//  str1 = Serial2.readString();
//  Serial.println("Serial2 Event: "); Serial.println(str1);  
//  Serial.write(Serial2.read());
//  }
//  wifi.events();
//}
