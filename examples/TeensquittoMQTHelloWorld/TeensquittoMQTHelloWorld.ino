#include <teensquitto.h>
#include <IPAddress.h>

teensquitto mqtt = teensquitto("MQTT", &Serial1);

void setup() {
  Serial.begin(1);
  Serial1.begin(2000000);

  ESP.onDetect([]() {
    IPAddress ip(192, 168, 1, xxx);
    IPAddress gate(192, 168, 1, 1);
    IPAddress sub(255, 255, 255, 0);
    WiFi.mode(WIFI_STA);
    WiFi.begin("your_ssid", "your_password");
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
    IPAddress _server = { 192, 168, 1, 5};  //this is the server address
                                            //ip of where you have mosquitto running
    mqtt.setKeepAlive(8);
    mqtt.setMaxTopicLength(128);
    mqtt.setClientId("Mike");
    //User Name, password
    mqtt.setCredentials("ESP12E", "mike");
    mqtt.setServer(_server, 1883);
    mqtt.connect();
  }); // end of onDetect callback
}

void loop() {
  static uint32_t timer = millis();
  if ( millis() - timer > 100 ) {
    timer = millis();
    static bool toggle_led; toggle_led = !toggle_led; pinMode(13, OUTPUT); digitalWrite(13, toggle_led);
    mqtt.publish("teensquitto/data1", 2, 1, "hello world"); 
  }
  WiFi.events();
}

void serialEvent1() {
  WiFi.events();
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
