#include <teensquitto.h>
#include <IPAddress.h>
#include <EEPROM.h>
#include <string>

#include "globals.h"
#include "MPU9250.h"
#include "MargUpdateFilter.h"
double MST_PrintVals[4];

  //--------------------------------------------------------------------
  // IMU Declares
  //--------------------------------------------------------------------
  #define IMU_BUS      Wire  //Wire // SPI
  #define IMU_ADDR     0x68  //0x69 // 0x68 // SPI 9
  #define IMU_SCL       19  //47 // 0x255
  #define IMU_SDA       18  //48 // 0x255
  #define IMU_SPD    1000000   //1000000 // 0==NULL or other
  #define IMU_SRD         9  // Used in initIMU setSRD() - setting SRD to 9 for a 100 Hz update rate
  #define IMU_INT_PIN    16  //50 // 1 // 16

// an MPU9250 object with the MPU-9250 sensor on I2C bus 0 with address 0x68
#if defined(TEENSYDUINO)
  MPU9250 Imu(IMU_BUS, IMU_ADDR, IMU_SCL, IMU_SDA, IMU_SPD);
#else
  MPU9250 Imu(IMU_BUS, IMU_ADDR);
#endif
int status;

//WiFi-UDP settings
    teensquitto UdpS = teensquitto("WiFiUdp", &Serial1);

    IPAddress ip(192, 168, 1, 159);
    IPAddress gate(192, 168, 1, 1);
    IPAddress sub(255, 255, 255, 0);
    const char* ssid = "..............";
    const char* password = "...........";

    //WiFiUDP Udp;
    unsigned int localUdpPort = 4210;  // local port to listen on
    unsigned int localUdpPortS = 3333;  // local port to listen on
    char incomingPacket[255];  // buffer for incoming packets
    char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back
    char  payload[255]; 

void setup()
{
      Serial.begin(115200);
      Serial1.begin(2000000);
      delay(1000);

  // start communication with IMU 
  status = Imu.begin();
  if (status < 0) {
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.print("Status: ");
    Serial.println(status);
    while(1) {}
  }

  initIMU();
  
  // quaternion of sensor frame relative to auxiliary frame
  quant[0] = 1.0f; quant[1] = 0.0f; 
  quant[2] = 0.0f; quant[3] = 0.0f;
  //Connecting ESP
  Serial.println();
  Serial.printf("Connecting to %s ", ssid);

  ESP.onDetect([]() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.waitForConnectResult();
    WiFi.config(ip, gate, sub);
    
    if (  udp.listen(localUdpPort) ) {
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
    UdpS.begin(localUdpPortS);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPortS);
  }); // end of onDetect callback
}

void loop()
{
  // read the sensor
  if ((newIMUData == 1)) {
    newIMUData = 0;
    getIMU();
    getPacket();

    UdpS.beginPacket(UdpS.remoteIP(), UdpS.remotePort());
    UdpS.print(payload);
    UdpS.endPacket();
    delay(100);    
  }
  WiFi.events();
}

void serialEvent1() {
  WiFi.events();
}

// This routine is called via interrupt from the IMU
void runFilter() {
  newIMUData = 1;
}
