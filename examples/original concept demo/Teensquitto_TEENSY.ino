#include <teensquitto.h>
#include <IPAddress.h>

teensquitto amber = teensquitto("amber", &Serial1, "192.168.2.122:1883", "ESP02", "tony");
teensquitto mqtt = teensquitto("MQTT", &Serial1);
teensquitto client = teensquitto("WiFiClient", &Serial1);
teensquitto sclient = teensquitto("WiFiClientSecure", &Serial1);
teensquitto Udp = teensquitto("WiFiUdp", &Serial1);
teensquitto eep = teensquitto("EEPROM", &Serial1);
teensquitto webSocketClient = teensquitto("WSC", &Serial1);
teensquitto webSocketServer = teensquitto("WSS", &Serial1);


void setup() {
  Serial.begin(1);
  Serial1.begin(2000000);
  Serial2.begin(921600);
  SPI2.begin();
  delay(2000);

  ESP.onDetect([]() {
    IPAddress ip(192, 168, 2, 168);
    IPAddress gate(192, 168, 2, 1);
    IPAddress sub(255, 255, 255, 0);
    WiFi.mode(WIFI_STA);
    WiFi.begin("Tony", "xxxxxxxxxx");
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
    IPAddress _server = { 192, 168, 2, 122 };
    mqtt.setKeepAlive(8);
    mqtt.setMaxTopicLength(128);
    mqtt.setClientId("Tony");
    mqtt.setCredentials("ESP02", "tony");
    mqtt.setServer(_server, 1883);
    mqtt.connect();

    webSocketClient.onEvent(webSocketEventClient);
    webSocketServer.onEvent(webSocketEventServer);

    webSocketServer.begin(); webSocketServer.setAuthorization("tony", "teensy");
    webSocketClient.disconnect();
    //  webSocketClient.begin("192.168.2.55", 43110, "/");
    // webSocketClient.setAuthorization("tony", "teensy");
    webSocketClient.setReconnectInterval(5000);

    if (  udp.listen(1234) ) {
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

}

void webSocketEventServer(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        Serial.println("connected server");
        //                IPAddress ip = webSocketServer.remoteIP(num);
        //                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        //  webSocketServer.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      //      hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }
}
void webSocketEventClient(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
        Serial.printf("[WSc] Connected to url: %s\n", payload);

        // send message to server when Connected
        //  webSocketClient.sendTXT("Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      //  hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
  }
}



void onMqttConnect(bool sessionPresent) {
  mqtt.subscribe("teensquitto/#", 2);
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
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";
const uint8_t fingerprintb[] = { 0xCF, 0x05, 0x98, 0x89, 0xCA, 0xFF, 0x8E, 0xD8, 0x5E, 0x5C, 0xE0, 0xC2, 0xE4, 0xF7, 0xE6, 0xC3, 0xC7, 0x50, 0xDD, 0x5C };
void loop() {
  //  delay(100); Serial.print("MILLIS(): "); Serial.println(millis());






//  int n = WiFi.scanNetworks();
//  Serial.println("scan done");
//  if (n == 0) {
//    Serial.println("no networks found");
//  }
//
//  else {
//    Serial.print(n);
//    Serial.println(" networks found");
//    for (int i = 0; i < n; ++i) {
//      // Print SSID and RSSI for each network found
//      Serial.print(i + 1);
//      Serial.print(": ");
//      Serial.print(WiFi.SSID(i));
//      Serial.print(" (");
//      Serial.print(WiFi.RSSI(i));
//      Serial.print(")");
//      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
//      delay(10);
//    }
//    Serial.println("");
//  }
//  for ( uint8_t i = 0; i < 255; i++ )  WiFi.events();
//
//  delay(5000);
//
//
//
//  return;














  static uint32_t timer2 = millis();
  if ( millis() - timer2 > 2000 ) {
    timer2 = millis();
    //  uint32_t _time = micros();
    //    teensy_gpio.digitalWrite(13, !teensy_gpio.digitalRead(13));
    //    Serial.println(micros() - _time);
  }



  static uint32_t timer = millis();
  if ( millis() - timer > 1050 ) {
    timer = millis();

    static bool toggle_led; toggle_led = !toggle_led; pinMode(13, OUTPUT); digitalWrite(13, toggle_led);

    static bool conn_udp = 1;
    webSocketClient.sendTXT("Hello Server!, from Tony!");
           if ( conn_udp && udp.listen(1234) ) {
          webSocketServer.begin(); webSocketServer.setAuthorization("tony", "teensy");
          webSocketClient.disconnect();
          webSocketClient.begin("192.168.2.55", 43110, "/");
          webSocketClient.setAuthorization("tony", "teensy");
          webSocketClient.setReconnectInterval(5000);
          conn_udp = 0;
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

    mqtt.publish("teensquitto/", 2, 1, "hello world"); // WiFi.scanNetworksAsync(hndl, 1);
  }
  WiFi.events();
}

void serialEvent1() {
  WiFi.events();
}

void hndl(int networksFound) {
  Serial.print("Networks Found: "); Serial.println(networksFound);
}

//
//void serialEvent() {
//  static bool _DTR = Serial.dtr();
//  if ( _DTR != Serial.dtr() ) {
//    uint8_t ESP_RESET = 24, ESP_GPIO0 = 25;
//    static uint32_t _baud = 1;
//    _DTR = Serial.dtr();
//    if ( !_DTR ) {
//      delay(100); while ( Serial.available() > 0 ) Serial.read();
//    }
//    else {
//      if ( _baud != Serial.baud() ) {
//        _baud = Serial.baud(); pinMode(0, INPUT_DISABLE); Serial1.setRX(27); Serial1.begin(_baud);
//      }
//      delay(40); if ( Serial.peek() != 0xC0 ) return;
//      pinMode(ESP_RESET, OUTPUT); digitalWriteFast(ESP_RESET, 0);
//      pinMode(ESP_GPIO0, OUTPUT); digitalWriteFast(ESP_GPIO0, 0);
//      while ( Serial.available() > 0 ) Serial.read();
//      uint32_t _pulse = millis(), _count = 0, _program_timeout = millis();
//      while (1) {
//        if ( _baud != Serial.baud() ) {
//          _baud = Serial.baud(); Serial1.begin(_baud);
//        }
//        digitalWriteFast(ESP_RESET, !Serial.dtr());
//        if ( millis() - _program_timeout > 12000 ) {
//          pinMode(ESP_GPIO0, INPUT_DISABLE); digitalWriteFast(ESP_RESET, 0); delay(100); pinMode(ESP_RESET, INPUT_DISABLE);
//          Serial1.setRX(0); pinMode(27, INPUT_DISABLE); Serial1.begin(2000000); return;
//        }
//        while ( Serial.available() > 0 ) {
//          _pulse = millis(); _count++;
//          if ( _count > 3500 ) {
//            _program_timeout = millis(); pinMode(ESP_GPIO0, INPUT_DISABLE);
//            if ( millis() - _pulse > 500 ) {
//              pinMode(ESP_RESET, INPUT_DISABLE); ((*(volatile uint32_t *)0xE000ED0C) = (0x5FA0004));
//            }
//          }
//          Serial1.write(Serial.read());
//        }
//        if ( Serial1.available() > 0 ) Serial.write(Serial1.read());
//        if ( _count > 3500 ) {
//          _program_timeout = millis(); pinMode(ESP_GPIO0, INPUT_DISABLE);
//          if ( millis() - _pulse > 500 ) {
//            pinMode(ESP_RESET, INPUT_DISABLE); ((*(volatile uint32_t *)0xE000ED0C) = (0x5FA0004));
//          }
//        }
//      }
//    }
//  }
//}

