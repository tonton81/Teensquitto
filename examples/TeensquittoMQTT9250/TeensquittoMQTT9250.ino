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
teensquitto mqtt = teensquitto("MQTT", &Serial1);

    IPAddress ip(192, 168, 1, 5);
    IPAddress gate(192, 168, 1, 1);
    IPAddress sub(255, 255, 255, 0);
    const char* ssid = "........";
    const char* password = "...........";

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

    // setup MQTT handlers
    mqtt.onConnect(onMqttConnect);
    mqtt.onDisconnect(onMqttDisconnect);
    mqtt.onSubscribe(onMqttSubscribe);
    mqtt.onUnsubscribe(onMqttUnsubscribe);
    mqtt.onMessage(onMqttMessage);
    mqtt.onPublish(onMqttPublish);

    WiFi.onStationModeGotIP(onWifiConnect);
    WiFi.onStationModeDisconnected(onWifiDisconnect);

    // setup MQTT connection
    IPAddress _server = { 192, 168, 1, 253};
    mqtt.setKeepAlive(8);
    mqtt.setMaxTopicLength(255);
    mqtt.setClientId("Mike");
    //User Name, password
    mqtt.setCredentials("ESP12E", "mike");
    mqtt.setServer(_server, 1883);
    mqtt.connect();
  }); // end of onDetect callback
}

void loop()
{
  // read the sensor
  if ((newIMUData == 1)) {
    newIMUData = 0;
    getIMU();
    getPacket();
    
   static uint32_t timer = millis();
   if ( millis() - timer >= 100 ) {
     timer = millis();
     static bool toggle_led; toggle_led = !toggle_led; pinMode(13, OUTPUT); digitalWrite(13, toggle_led);
     mqtt.publish("teensquitto/ypr", 2, 1, payload); 
   }
   WiFi.events();
 }
}

void serialEvent1() {
  WiFi.events();
}

// This routine is called via interrupt from the IMU
void runFilter() {
  newIMUData = 1;
}

void onMqttConnect(bool sessionPresent) {
  mqtt.subscribe("teensquitto/data2", 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
}

void onMqttUnsubscribe(uint16_t packetId) {
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("****************************");
  Serial.println("Publish received.");
  Serial.print(" topic: "); Serial.print(topic);
  Serial.print(" payload: ");
  for ( uint32_t i = 0; i < len; i++ ) Serial.print(payload[i]); Serial.println();
  Serial.print(" qos: "); Serial.print(properties.qos); Serial.print(" dup: "); Serial.println(properties.dup);
  Serial.print(" retain: "); Serial.print(properties.retain);
  Serial.print(" len: "); Serial.print(len); Serial.print(" index: "); Serial.print(index); Serial.print(" total: "); Serial.println(total);
  Serial.println(""); Serial.println("");
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Packet ID: "); Serial.println(packetId);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
}
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("--------DISCONNECTED!!---------------");
  Serial.println(event.ssid);
  Serial.println(event.bssid[0]);
  Serial.println(event.bssid[1]);
  Serial.println(event.bssid[2]);
  Serial.println(event.bssid[3]);
  Serial.println(event.bssid[4]);
  Serial.println(event.bssid[5]);
  Serial.println(event.reason);
  Serial.println("-----------------------------------");
}
