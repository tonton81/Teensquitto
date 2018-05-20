/*
  MIT License
  Copyright (c) 2018 Antonio Alexander Brewer (tonton81) - https://github.com/tonton81
  Contributors:
  Tim - https://github.com/Defragster
  Mike - https://github.com/mjs513
  Designed and tested for PJRC Teensy 3.2, 3.5, and 3.6 boards.
  Forum link : https://forum.pjrc.com/threads/48839-New-project-code-named-teensquitto
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


#include <teensquitto.h>
#include "Stream.h"
#include <SPI.h>
#include <Wire.h>
#include <FastLED.h>
#include <IPAddress.h>
#include <ESPAsyncUDP.h>
#include <deque>
#include <queue>

std::deque<std::vector<uint8_t>> teensquitto::teensy_handler_queue;
volatile bool teensquitto::_mqtt_online_flag = 0;
uint8_t teensquitto::_wifi_status = 0;
Stream* teensquitto::deviceSerial = NULL;
uint8_t teensquitto::storage_buffer[512] = { 0 };
uint8_t teensquitto::_mac_buffer[6] = { 0 };
bool teensquitto::data_ready_flag = 0;
bool teensquitto::_debug_flag = 1;
volatile bool teensquitto::_mqtt_server_sync = 0;
bool teensquitto::_esp_detected = 1;
volatile bool teensquitto::_pendingData = 0;
volatile uint32_t teensquitto::_mqtt_server_latency = 0;
uint16_t teensquitto::_esp_latency = 0;
uint32_t teensquitto::_esp_check_latency = 0;
uint32_t teensquitto::_esp_sync_timeout = 0;
uint32_t teensquitto::_esp_sync_interval = 0;
uint8_t teensquitto::ack_nak_buf[4] = { 0 };
mqttConnect teensquitto::mqtt_connect = NULL;
mqttDisconnect teensquitto::mqtt_disconnect = NULL;
mqttSubscribe teensquitto::mqtt_sub = NULL;
mqttUnsubscribe teensquitto::mqtt_unsub = NULL;
mqttMessage teensquitto::mqtt_message = NULL;
mqttPublish teensquitto::mqtt_pub = NULL;
onStationModeGotIPPtr teensquitto::wifi_onSTAgotIP = NULL;
onStationModeDisconnectedPtr teensquitto::wifi_onSTADisconnect = NULL;
onStationModeConnectedPtr teensquitto::wifi_onSTAConnected = NULL;
onStationModeAuthModeChangedPtr teensquitto::wifi_onSTAAuthModeChange = NULL;
onStationModeDHCPTimeoutPtr teensquitto::wifi_onSTADHCPtimeout = NULL;
onSoftAPModeStationConnectedPtr teensquitto::wifi_onAPConnected = NULL;
onSoftAPModeStationDisconnectedPtr teensquitto::wifi_onAPDisconnected = NULL;
onSoftAPModeProbeRequestReceivedPtr teensquitto::wifi_onAPProbe = NULL;
asyncScanPtr teensquitto::wifi_Scan_hndl = NULL;
WSCPtr teensquitto::wsc_hndl = NULL;
WSSPtr teensquitto::wss_hndl = NULL;
espDetectPtr teensquitto::_espDetectHandler = NULL;

uint8_t teensquitto::_wifi_handlers = 0; // all 8 disabled
uint8_t teensquitto::_mqtt_handlers = 0; // all 6 disabled


teensquitto::teensquitto(const char *localName, Stream *UART, const char *connect_address, const char *mqttuser, const char *mqttpass) {
  deviceSerial = UART; _mqtt_support = 1;
  strlcpy(local_clientId, localName, 32); strlcpy(mqtt_user, mqttuser, 32); strlcpy(mqtt_pass, mqttpass, 32);
  for ( uint32_t i = 0; i < strlen(connect_address); i++ ) {
    char_ip[i] = connect_address[i]; 
    if ( connect_address[i] == 0x3A ) { char_ip[i] = '\0'; break; }
  }
  for ( uint32_t i = 0; i < 5; i++ ) {
    if ( i < 4 ) { mqtt_ip[i] = strtoul(connect_address, NULL, DEC); connect_address = strpbrk(connect_address, ":."); }
    else mqtt_port = strtoul(connect_address, NULL, DEC); connect_address++;
  }
}
teensquitto::teensquitto(const char *remoteName, const char *remoteLocation, Stream *UART, const char *peripheral ) {
  deviceSerial = UART;
}
teensquitto::teensquitto(const char *data, Stream *UART) {
       if ( !strcmp(data,"WiFi")       ) { _wifi = 1;                            }
  else if ( !strcmp(data,"WiFiUdp")    ) { _wifiudp = 1;                         }
  else if ( !strcmp(data,"WiFiClient") ) { _wificlient = 0;                      }
  else if ( !strcmp(data,"WiFiClientSecure") ) { _wificlientsecure = 0;          }
  else if ( !strcmp(data,"WiFiMulti")  ) { _wifimulti = 1;                       }
  else if ( !strcmp(data,"ESPWire")    ) { esp_wire = 1;                         }
  else if ( !strcmp(data,"ESPSPI")     ) { esp_spi  = 1;                         }
  else if ( !strcmp(data,"SPIFFS")     ) { _spiffs  = 1;                         }
  else if ( !strcmp(data,"MQTT")       ) { _mqtt_support  = 1;                   }
  else if ( !strcmp(data,"ESP")        ) { _esp  = 1;                            }
  else if ( !strcmp(data,"EEPROM")     ) { _esp_eeprom  = 1;                     }
  else if ( !strcmp(data,"WSC")        ) { _websocketclient = 1;                 }
  else if ( !strcmp(data,"WSS")        ) { _websocketserver = 1;                 }
  deviceSerial = UART; esp_control = 1;
}
teensquitto::teensquitto(const char *data) {
       if ( !strcmp(data,"WiFi")       ) { _wifi = 1;                            }
  else if ( !strcmp(data,"SPIFFS")     ) { _spiffs  = 1;                         }
  else if ( !strcmp(data,"ESP")        ) { _esp  = 1;                            }
  else if ( !strcmp(data,"WiFiMulti")  ) { _WiFiMulti = 1;                       }
}

bool teensquitto::open(const String& path, const char* mode) {
  return open(path.c_str(), mode);
}
bool teensquitto::open(const char* path, const char* mode) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(path) + strlen(mode) + 9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++; // SPIFFS OPEN
    data[data_pos] = strlen(path); checksum ^= data[data_pos]; data_pos++; // SPIFFS PATH
    data[data_pos] = strlen(mode); checksum ^= data[data_pos]; data_pos++; // SPIFFS MODE (R/W)
    for ( uint32_t i = 0; i < strlen(path); i++ ) { data[data_pos] = path[i]; checksum ^= path[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(mode); i++ ) { data[data_pos] = mode[i]; checksum ^= mode[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::exists(const String& path) {
  return exists(path.c_str());
}
bool teensquitto::exists(const char* path) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(path) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++; // SPIFFS EXISTS
    data[data_pos] = strlen(path); checksum ^= data[data_pos]; data_pos++; // SPIFFS PATH
    for ( uint32_t i = 0; i < strlen(path); i++ ) { data[data_pos] = path[i]; checksum ^= path[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::remove(const String& path) {
  return exists(path.c_str());
}
bool teensquitto::remove(const char* path) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(path) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++; // SPIFFS REMOVE
    data[data_pos] = strlen(path); checksum ^= data[data_pos]; data_pos++; // SPIFFS PATH
    for ( uint32_t i = 0; i < strlen(path); i++ ) { data[data_pos] = path[i]; checksum ^= path[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::format() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++; // SPIFFS FORMAT
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 35000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 35000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::rename(const String& pathFrom, const String& pathTo) {
    return rename(pathFrom.c_str(), pathTo.c_str());
}
bool teensquitto::rename(const char* pathFrom, const char* pathTo) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(pathFrom) + strlen(pathTo) + 9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++; // SPIFFS OPEN
    data[data_pos] = strlen(pathFrom); checksum ^= data[data_pos]; data_pos++; // SPIFFS PATHFROM RENAME
    data[data_pos] = strlen(pathTo); checksum ^= data[data_pos]; data_pos++; // SPIFFS PATHTO RENAME
    for ( uint32_t i = 0; i < strlen(pathFrom); i++ ) { data[data_pos] = pathFrom[i]; checksum ^= pathFrom[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(pathTo); i++ ) { data[data_pos] = pathTo[i]; checksum ^= pathTo[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
String teensquitto::readStringUntil(char terminator) {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++; // SPIFFS READSTRINGUNTIL
    data[data_pos] = terminator; checksum ^= data[data_pos]; data_pos++; // SPIFFS TERMINATOR CHAR
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1500UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[(((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]) + 1)];
      for ( uint16_t i = 0; i < ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); i++ ) response[i] = storage_buffer[7+i];
      response[((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6])] = '\0'; return String(response);
    }
  }
  return "";
}
bool teensquitto::close() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++; // SPIFFS CLOSE FILE
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
size_t teensquitto::position() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++; // SPIFFS POSITION()
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t result = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return result;
    }
  }
  return 0;
}
size_t teensquitto::size() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++; // SPIFFS SIZE()
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t result = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return result;
    }
  }
  return 0;
}
String teensquitto::name() {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++; // SPIFFS FILE NAME
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      response[storage_buffer[5]] = '\0'; return String(response);
    }
  }
  return "";
}
size_t teensquitto::read(uint8_t* buf, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[11], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++; // SPIFFS READ(BUF,LEN)
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 2500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 2500UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buf[i] = storage_buffer[9+i]; return _size;
    }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[12], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++; // 
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 2500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 2500UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buf[i] = storage_buffer[9+i]; return _size;
    }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++; // 
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 2500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 2500UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buf[i] = storage_buffer[9+i]; return _size;
    }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[11], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++; // UDP READ(BUF,LEN)
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 2500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 2500UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buf[i] = storage_buffer[9+i]; return _size;
    }
  }
  return 0;
}
bool teensquitto::seek(uint32_t pos, SeekMode mode) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[12], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++; // SPIFFS SEEK
    data[data_pos] = ((uint8_t)(pos >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(pos >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(pos >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)pos); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = mode; checksum ^= data[data_pos]; data_pos++; // SPIFFS SEEK MODE
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::peek() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++; // SPIFFS PEEK
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
int teensquitto::available() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++; // SPIFFS AVAILABLE
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      return (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
    }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      return (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
    }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      return (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
    }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      return (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
    }
  }
  return 0;
}
int teensquitto::read() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++; // SPIFFS READ
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((int16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
void teensquitto::flush() {
  if ( !_esp_detected ) return;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++; // SPIFFS FLUSH
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++; // WIFICLIENT FLUSH
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++; // WIFICLIENT FLUSH
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
size_t teensquitto::write(const uint8_t *buf, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[11+((uint32_t)size)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++; // SPIFFS WRITE(BUF,LEN)
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = buf[i]; checksum ^= data[data_pos]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return _size;
    }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[12+((uint32_t)size)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++; // WIFICLIENT WRITE(BUF,LEN)
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = buf[i]; checksum ^= data[data_pos]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return _size;
    }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[11+((uint32_t)size)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++; // UDP WRITE(BUF,LEN)
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = buf[i]; checksum ^= data[data_pos]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return _size;
    }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11+((uint32_t)size)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++; // WIFICLIENTSECURE WRITE(BUF,LEN)
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = buf[i]; checksum ^= data[data_pos]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]; return _size;
    }
  }
  return 0;
}
bool teensquitto::openDir(const char* path) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(path) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++; // SPIFFS OPENDIR
    data[data_pos] = strlen(path); checksum ^= data[data_pos]; data_pos++; // PATH
    for ( uint32_t i = 0; i < strlen(path); i++ ) { data[data_pos] = path[i]; checksum ^= path[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return 1; }
  }
  return 0;
}
bool teensquitto::next() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x19; checksum ^= data[data_pos]; data_pos++; // SPIFFS NEXT
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
String teensquitto::fileName() {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x20; checksum ^= data[data_pos]; data_pos++; // SPIFFS OPENDIR
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[(((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]) + 1)];
      for ( uint16_t i = 0; i < ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); i++ ) response[i] = storage_buffer[7+i];
      response[((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6])] = '\0'; return String(response);
    }
  }
  return "";
}
size_t teensquitto::fileSize() {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x21; checksum ^= data[data_pos]; data_pos++; // SPIFFS FLUSH
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]);
    }
  }
  return 0;
}
bool teensquitto::openFile(const char* mode) {
  if ( !_esp_detected ) return 0;
  if ( _spiffs ) {
    uint8_t data[strlen(mode) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x22; checksum ^= data[data_pos]; data_pos++; // SPIFFS OPENFILE
    data[data_pos] = strlen(mode); checksum ^= data[data_pos]; data_pos++; // MODE
    for ( uint32_t i = 0; i < strlen(mode); i++ ) { data[data_pos] = mode[i]; checksum ^= mode[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::info(FSInfo& info){
  if ( !_esp_detected ) {
    info.totalBytes = 0;
    info.usedBytes = 0;
    info.blockSize = 0;
    info.pageSize = 0;
    info.maxOpenFiles = 0;
    info.maxPathLength = 0;
    return 0;
  }
  if ( _spiffs ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x23; checksum ^= data[data_pos]; data_pos++; // SPIFFS (FS)INFO
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      info.totalBytes = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      info.usedBytes = (uint32_t)(storage_buffer[9] << 24) | storage_buffer[10] << 16 | storage_buffer[11] << 8 | storage_buffer[12];
      info.blockSize = (uint32_t)(storage_buffer[13] << 24) | storage_buffer[14] << 16 | storage_buffer[15] << 8 | storage_buffer[16];
      info.pageSize = (uint32_t)(storage_buffer[17] << 24) | storage_buffer[18] << 16 | storage_buffer[19] << 8 | storage_buffer[20];
      info.maxOpenFiles = (uint32_t)(storage_buffer[21] << 24) | storage_buffer[22] << 16 | storage_buffer[23] << 8 | storage_buffer[24];
      info.maxPathLength = (uint32_t)(storage_buffer[25] << 24) | storage_buffer[26] << 16 | storage_buffer[27] << 8 | storage_buffer[28];
      return storage_buffer[29];
    }
  }
  return 0;
}
uint16_t teensquitto::subscribe(const char* topic, uint8_t qos) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[strlen(topic) + 9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = qos; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
uint16_t teensquitto::unsubscribe(const char* topic) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[strlen(topic) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
bool teensquitto::connected(){
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::connect(){
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint16_t teensquitto::publish(const char* topic, uint8_t qos, bool retain, const char* payload, size_t length, bool dup, uint16_t message_id)  {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support && payload == nullptr ) {
    uint8_t data[strlen(topic)+10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = qos; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = retain; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _mqtt_support && payload != nullptr ) {
    uint8_t data[strlen(topic)+strlen(payload)+18], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(payload); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = qos; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = retain; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = dup; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = message_id >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = message_id; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(payload); i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; data_pos++; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
void teensquitto::onConnect(mqttConnect handler){
   if ( _mqtt_support ) { mqtt_connect = handler; bitSet(_mqtt_handlers, 0); update_mqtt_handlers(); }
}
void teensquitto::onDisconnect(mqttDisconnect handler){
   if ( _mqtt_support ) { mqtt_disconnect = handler; bitSet(_mqtt_handlers, 1); update_mqtt_handlers(); }
}
void teensquitto::onSubscribe(mqttSubscribe handler){
   if ( _mqtt_support ) { mqtt_sub = handler; bitSet(_mqtt_handlers, 2); update_mqtt_handlers(); }
}
void teensquitto::onUnsubscribe(mqttUnsubscribe handler){
   if ( _mqtt_support ) { mqtt_unsub = handler; bitSet(_mqtt_handlers, 3); update_mqtt_handlers(); }
}
void teensquitto::onMessage(mqttMessage handler){
   if ( _mqtt_support ) { mqtt_message = handler; bitSet(_mqtt_handlers, 4); update_mqtt_handlers(); }
}
void teensquitto::onPublish(mqttPublish handler){
   if ( _mqtt_support ) { mqtt_pub = handler; bitSet(_mqtt_handlers, 5); update_mqtt_handlers(); }
}
void teensquitto::update_mqtt_handlers() {
  uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = _mqtt_handlers; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; // write crc
  deviceSerial->write(data, sizeof(data));
}
void teensquitto::onStationModeGotIP(onStationModeGotIPPtr handler){
   if ( _wifi != 255 ) { wifi_onSTAgotIP = handler; bitSet(_wifi_handlers, 0); update_wifi_handlers(); }
}
void teensquitto::onStationModeDisconnected(onStationModeDisconnectedPtr handler){
   if ( _wifi != 255 ) { wifi_onSTADisconnect = handler; bitSet(_wifi_handlers, 1); update_wifi_handlers(); }
}
void teensquitto::onStationModeConnected(onStationModeConnectedPtr handler){
   if ( _wifi != 255 ) { wifi_onSTAConnected = handler; bitSet(_wifi_handlers, 2); update_wifi_handlers(); }
}
void teensquitto::onStationModeAuthModeChanged(onStationModeAuthModeChangedPtr handler){
   if ( _wifi != 255 ) { wifi_onSTAAuthModeChange = handler; bitSet(_wifi_handlers, 3); update_wifi_handlers(); }
}
void teensquitto::onStationModeDHCPTimeout(onStationModeDHCPTimeoutPtr handler){
   if ( _wifi != 255 ) { wifi_onSTADHCPtimeout = handler; bitSet(_wifi_handlers, 4); update_wifi_handlers(); }
}
void teensquitto::onSoftAPModeStationConnected(onSoftAPModeStationConnectedPtr handler){
   if ( _wifi != 255 ) { wifi_onAPConnected = handler; bitSet(_wifi_handlers, 5); update_wifi_handlers(); }
}
void teensquitto::onSoftAPModeStationDisconnected(onSoftAPModeStationDisconnectedPtr handler){
   if ( _wifi != 255 ) { wifi_onAPDisconnected = handler; bitSet(_wifi_handlers, 6); update_wifi_handlers(); }
}
void teensquitto::onSoftAPModeProbeRequestReceived(onSoftAPModeProbeRequestReceivedPtr handler){
   if ( _wifi != 255 ) { wifi_onAPProbe = handler; bitSet(_wifi_handlers, 7); update_wifi_handlers(); }
}
void teensquitto::update_wifi_handlers() {
  uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x45; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = _wifi_handlers; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; // write crc
  deviceSerial->write(data, sizeof(data));
}

void teensquitto::onDetect(espDetectPtr handler) {
  if ( _esp != 255 ) {
    _espDetectHandler = handler; std::bind(&teensquitto::_onDetect, this);
    if ( _esp_detected && _espDetectHandler != nullptr ) _espDetectHandler();
  }
}
void teensquitto::_onDetect() { _espDetectHandler(); }





bool teensquitto::setKeepAlive(uint16_t keepAlive) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = keepAlive>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = keepAlive; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setClientId(const char* clientId) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[strlen(clientId) + 8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(clientId); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(clientId); i++ ) { data[data_pos] = clientId[i]; checksum ^= clientId[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setCleanSession(bool cleanSession) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = cleanSession; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setMaxTopicLength(uint16_t maxTopicLength) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = maxTopicLength>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = maxTopicLength; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setCredentials(const char* username, const char* password) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support && password == nullptr ) {
    uint8_t data[strlen(username)+8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(username); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(username); i++ ) { data[data_pos] = username[i]; checksum ^= username[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _mqtt_support && password != nullptr ) {
    uint8_t data[strlen(username)+strlen(password)+9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(username); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(password); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(username); i++ ) { data[data_pos] = username[i]; checksum ^= username[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(password); i++ ) { data[data_pos] = password[i]; checksum ^= password[i]; data_pos++; }
    data[data_pos] = checksum; data_pos++; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setServer(const char* host, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[strlen(host) + 10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setServer(IPAddress ip, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support ) {
    uint8_t data[13], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[0]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[1]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[2]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[3]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setWill(const char* topic, uint8_t qos, bool retain, const char* payload, size_t length) {
  if ( !_esp_detected ) return 0;
  if ( _mqtt_support && payload == nullptr ) {
    uint8_t data[strlen(topic)+10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = qos; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = retain; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
  }
  if ( _mqtt_support && payload != nullptr ) {
    uint8_t data[strlen(topic)+strlen(payload)+15], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(topic); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(payload); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = qos; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = retain; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(topic); i++ ) { data[data_pos] = topic[i]; checksum ^= topic[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(payload); i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; data_pos++; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::begin(char* ssid, char *passphrase, int32_t channel, const uint8_t* bssid, bool connect) {
  if ( _wifi != 255 ) return begin((const char*) ssid, (const char*) passphrase, channel, bssid, connect);
  return 0;
}
uint8_t teensquitto::begin(const char* ssid, const char *passphrase, int32_t channel, const uint8_t* bssid, bool connect) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    if ( passphrase == NULL ) {
      uint8_t data[strlen(ssid)+8], checksum = 0; uint16_t data_pos = 0;
      data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
      data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
      data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
      for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
      data[data_pos] = checksum; // write crc
      deviceSerial->write(data, sizeof(data));
      uint32_t timeout = millis(); data_ready_flag = 0;
      while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
      if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
      if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
      return 0;
    }
    if ( bssid == NULL ) {
      char buf[4]; memcpy(buf, &channel, 4);
      uint8_t data[strlen(ssid)+strlen(passphrase)+13], checksum = 0; uint16_t data_pos = 0;
      data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
      data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
      data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(passphrase); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = buf[0]; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = buf[1]; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = buf[2]; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = buf[3]; checksum ^= data[data_pos]; data_pos++;
      for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
      for ( uint32_t i = 0; i < strlen(passphrase); i++ ) { data[data_pos] = passphrase[i]; checksum ^= passphrase[i]; data_pos++; }
      data[data_pos] = checksum; // write crc
      deviceSerial->write(data, sizeof(data));
      uint32_t timeout = millis(); data_ready_flag = 0;
      while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
      if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
      if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
      return 0;
    }
    char buf[4]; memcpy(buf, &channel, 4);
    uint8_t data[strlen(ssid)+strlen(passphrase)+20], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(passphrase); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = buf[0]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = buf[1]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = buf[2]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = buf[3]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[0]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[1]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[2]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[3]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[4]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = bssid[5]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = connect; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(passphrase); i++ ) { data[data_pos] = passphrase[i]; checksum ^= passphrase[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
    return 0;
  }
  return 0;
}
int8_t teensquitto::begin() { // I2C/SPI/WiFi/SPIFFS/AsyncWSS
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x34; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++; // SPIFFS BEGIN
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _websocketserver != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    return 1; // actually supposed to be a void function
  }
  return 0;
}
bool teensquitto::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[27], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++; // CONFIG
    data[data_pos] = IPAddress(local_ip)[0]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[1]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[2]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[3]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(gateway)[0]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[1]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[2]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[3]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(subnet)[0]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[1]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[2]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[3]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(dns1)[0]; checksum ^= data[data_pos]; data_pos++; // DNS1
    data[data_pos] = IPAddress(dns1)[1]; checksum ^= data[data_pos]; data_pos++; // DNS1
    data[data_pos] = IPAddress(dns1)[2]; checksum ^= data[data_pos]; data_pos++; // DNS1
    data[data_pos] = IPAddress(dns1)[3]; checksum ^= data[data_pos]; data_pos++; // DNS1
    data[data_pos] = IPAddress(dns2)[0]; checksum ^= data[data_pos]; data_pos++; // DNS2
    data[data_pos] = IPAddress(dns2)[1]; checksum ^= data[data_pos]; data_pos++; // DNS2
    data[data_pos] = IPAddress(dns2)[2]; checksum ^= data[data_pos]; data_pos++; // DNS2
    data[data_pos] = IPAddress(dns2)[3]; checksum ^= data[data_pos]; data_pos++; // DNS2
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::disconnect(bool wifioff) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++; // DISCONNECT
    data[data_pos] = wifioff; checksum ^= data[data_pos]; data_pos++; // OPTIONAL TURN WIFI OFF
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _mqtt_support ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0x38; checksum = data[data_pos]; data_pos++; data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = wifioff; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _websocketclient != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    return 1; // void function for websockets library
  }
  if ( _websocketserver != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x19; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = wifioff; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    return 1; // void function for websockets library
  }
  return 0;
}
bool teensquitto::reconnect() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setAutoConnect(bool autoConnect) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = autoConnect; checksum ^= data[data_pos]; data_pos++; // setAutoConnect();
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::getAutoConnect() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setAutoReconnect(bool autoReconnect) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = autoReconnect; checksum ^= data[data_pos]; data_pos++; // setAutoReconnect();
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::waitForConnectResult() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 3500UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::isConnected() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
IPAddress teensquitto::localIP() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
IPAddress teensquitto::remoteIP() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
IPAddress teensquitto::destinationIP() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifiudp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
IPAddress teensquitto::subnetMask() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
IPAddress teensquitto::gatewayIP() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
IPAddress teensquitto::dnsIP(uint8_t dns_no) {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = dns_no; checksum ^= data[data_pos]; data_pos++; // dnsIP();
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
String teensquitto::macAddress() {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
} 
String teensquitto::hostname(void) {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}
bool teensquitto::hostname(char* aHostname) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    if(strlen(aHostname) > 32) return 0;
    uint8_t data[strlen(aHostname) + 8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(aHostname); checksum ^= data[data_pos]; data_pos++; // aHostname_length
    for ( uint32_t i = 0; i < strlen(aHostname); i++ ) { data[data_pos] = aHostname[i]; checksum ^= aHostname[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::hostname(const char* aHostname) {
  if ( _wifi != 255 ) return hostname((char*) aHostname);
  return 0;
}

bool teensquitto::hostname(String aHostname) {
  if ( _wifi != 255 ) return hostname((char*) aHostname.c_str());
  return 0;
}
String teensquitto::SSID() {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x19; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}
String teensquitto::psk() {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x20; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}    
String teensquitto::BSSIDstr() {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x21; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
} 
int32_t teensquitto::RSSI() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x22; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[4] = { storage_buffer[5], storage_buffer[6], storage_buffer[7], storage_buffer[8] }; int32_t val; memcpy(&val, buf, 4); return val; }
  }
  return 0;
}
uint8_t teensquitto::status() {
Serial.println("STATUS CALLED!");
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x23; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlient != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t* teensquitto::macAddress(uint8_t* mac) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x24; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return mac; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { mac[0] = storage_buffer[5]; mac[1] = storage_buffer[6]; mac[2] = storage_buffer[7]; mac[3] = storage_buffer[8]; mac[4] = storage_buffer[9]; mac[5] = storage_buffer[10]; return mac; }
  }
  return mac;
}
uint8_t* teensquitto::BSSID() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x25; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { _mac_buffer[0] = storage_buffer[5]; _mac_buffer[1] = storage_buffer[6]; _mac_buffer[2] = storage_buffer[7]; _mac_buffer[3] = storage_buffer[8]; _mac_buffer[4] = storage_buffer[9]; _mac_buffer[5] = storage_buffer[10]; return _mac_buffer; }
  }
  return 0;
}

bool teensquitto::beginSmartConfig() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x26; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::stopSmartConfig() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x27; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::smartConfigDone() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x28; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::beginWPSConfig(void) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x29; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::reset(void) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::restart(void) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::wdtEnable(uint32_t timeout_ms) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[11], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(timeout_ms >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(timeout_ms >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(timeout_ms >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)timeout_ms); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::wdtEnable(WDTO_t timeout_ms) {
    wdtEnable((uint32_t) timeout_ms);
}
void teensquitto::wdtDisable(void) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::wdtFeed(void) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::deepSleep(uint32_t time_us, WakeMode mode) {
  if ( !_esp_detected ) return;
  if ( _esp != 255 ) {
    uint8_t data[12], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time_us >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time_us >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time_us >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)time_us); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = (uint8_t)mode; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::rtcUserMemoryRead(uint32_t offset, uint32_t *rtcdata, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[15+(((uint32_t)size)*4)], checksum = 0; uint32_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)offset); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < (uint32_t)size; i++ ) {
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 24)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 16)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 8)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)rtcdata[i]); checksum ^= data[data_pos]; data_pos++;
    }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
return 0;
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {



      return storage_buffer[5];
    }
  }
  return 0;
}
bool teensquitto::rtcUserMemoryWrite(uint32_t offset, uint32_t *rtcdata, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[15+(((uint32_t)size)*4)], checksum = 0; uint32_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(offset >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)offset); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < (uint32_t)size; i++ ) {
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 24)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 16)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)(rtcdata[i] >> 8)); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = ((uint8_t)rtcdata[i]); checksum ^= data[data_pos]; data_pos++;
    }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
return 0;
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {



      return storage_buffer[5];
    }
  }
  return 0;
}
uint16_t teensquitto::getVcc(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
uint32_t teensquitto::getFreeHeap(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint32_t teensquitto::getChipId(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint8_t teensquitto::getBootVersion(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::getBootMode(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::getCpuFreqMHz(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint32_t teensquitto::getFlashChipId(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
String teensquitto::getCoreVersion() {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++; // GETCOREVERSION
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      response[storage_buffer[5]] = '\0'; return String(response);
    }
  }
  return "";
}
const char * teensquitto::getSdkVersion(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      const char* _response = response;
      response[storage_buffer[5]] = '\0'; return _response;
    }
  }
  return "";
}
uint32_t teensquitto::getFlashChipRealSize(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint32_t teensquitto::getFlashChipSize(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint32_t teensquitto::getFlashChipSpeed(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x19; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint8_t teensquitto::getFlashChipMode(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x20; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint32_t teensquitto::getFlashChipSizeByChipId(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x21; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint32_t teensquitto::magicFlashChipSize(uint8_t byte) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x22; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = byte; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint32_t teensquitto::magicFlashChipSpeed(uint8_t byte) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x23; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = byte; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
uint8_t teensquitto::magicFlashChipMode(uint8_t byte) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x24; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = byte; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::checkFlashConfig(bool needsEquals) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x25; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = needsEquals; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::flashEraseSector(uint32_t sector) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[11], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x26; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sector >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sector >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sector >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)sector); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::flashWrite(uint32_t offset, uint32_t *data, size_t size) {
// 27
  return 0;
}
bool teensquitto::flashRead(uint32_t offset, uint32_t *data, size_t size) {
// 28
  return 0;
}
uint32_t teensquitto::getSketchSize() {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x29; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
String teensquitto::getSketchMD5() {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x30; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      response[storage_buffer[5]] = '\0'; return String(response);
    }
  }
  return "";
}
uint32_t teensquitto::getFreeSketchSpace() {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x31; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
bool teensquitto::updateSketch(Stream& in, uint32_t size, bool restartOnFail, bool restartOnSuccess) {
// 32
  return 0;
}
String teensquitto::getResetReason(void) {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x33; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      response[storage_buffer[5]] = '\0'; return String(response);
    }
  }
  return "";
}
String teensquitto::getResetInfo(void) {
  if ( !_esp_detected ) return "";
  if ( _spiffs ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char response[storage_buffer[5] + 1];
      for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) response[i] = storage_buffer[6+i];
      response[storage_buffer[5]] = '\0'; return String(response);
    }
  }
  return "";
}
struct rst_info * teensquitto::getResetInfoPtr(void) {
//  35  return &resetInfo;
}
bool teensquitto::eraseConfig(void) {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x36; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint32_t teensquitto::getCycleCount() {
  if ( !_esp_detected ) return 0;
  if ( _esp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA4; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA4; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x37; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8]); }
  }
  return 0;
}
int teensquitto::connect(const String host, uint16_t port) { return connect(host.c_str(), port); }
int teensquitto::connect(const char* host, uint16_t port) {
  if ( _wificlient != 255 ) {
    uint8_t data[strlen(host) + 11], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[strlen(host) + 10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::connect(IPAddress ip, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[14], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[0]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[1]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[2]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[3]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[13], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[0]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[1]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[2]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[3]; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::verify(const char* fp, const char* domain_name) {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[9+strlen(fp)+strlen(domain_name)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(fp); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(domain_name); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(fp); i++ ) { data[data_pos] = fp[i]; checksum ^= fp[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(domain_name); i++ ) { data[data_pos] = domain_name[i]; checksum ^= domain_name[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::verifyCertChain(const char* domain_name) {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[8+strlen(domain_name)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(domain_name); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(domain_name); i++ ) { data[data_pos] = domain_name[i]; checksum ^= domain_name[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
size_t teensquitto::peekBytes(uint8_t *buffer, size_t length) {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[12], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));    
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 5000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 5000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buffer[i] = storage_buffer[9+i]; return _size;
    }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));    
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 5000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 5000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      uint32_t _size = (uint32_t)(storage_buffer[5] << 24) | storage_buffer[6] << 16 | storage_buffer[7] << 8 | storage_buffer[8];
      for ( uint32_t i = 0; i < _size; i++ ) buffer[i] = storage_buffer[9+i]; return _size;
    }
  }
  return 0;
}
bool teensquitto::stop() {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return 0;
}
bool teensquitto::getNoDelay() {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setNoDelay(bool nodelay) {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint16_t teensquitto::remotePort() {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
uint16_t teensquitto::localPort() {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
void teensquitto::stopAll() {
  if ( !_esp_detected ) return;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::setLocalPortStart(uint16_t port) {
  if ( !_esp_detected ) return;
  if ( _wificlient != 255 ) {
    uint8_t data[10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x19; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
int teensquitto::availableForWrite() {
  if ( !_esp_detected ) return 0;
  if ( _wificlient != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA6; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA6; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x20; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = _wificlient; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
int teensquitto::hostByName(const char* aHostname, IPAddress& aResult) {
  return hostByName(aHostname, aResult, 10000);
}
int teensquitto::hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[strlen(aHostname) + 12], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x30; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(aHostname); checksum ^= data[data_pos]; data_pos++; // aHostname_length
    data[data_pos] = ((uint8_t)(timeout_ms >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(timeout_ms >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(timeout_ms >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)timeout_ms); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(aHostname); i++ ) { data[data_pos] = aHostname[i]; checksum ^= aHostname[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { aResult = IPAddress(0, 0, 0, 0); return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { aResult = IPAddress(storage_buffer[6], storage_buffer[7], storage_buffer[8], storage_buffer[9]); return storage_buffer[5]; }
  }
  aResult = IPAddress(0, 0, 0, 0); return 0;
}
bool teensquitto::getPersistent(){
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x31; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int32_t teensquitto::channel(void) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x32; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[4] = { storage_buffer[5], storage_buffer[6], storage_buffer[7], storage_buffer[8] }; int32_t val; memcpy(&val, buf, 4); return val; }
  }
  return 0;
}
bool teensquitto::setSleepMode(uint8_t type) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x33; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = type; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::getSleepMode() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x34; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setPhyMode(uint8_t mode) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x35; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = mode; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::getPhyMode() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x36; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::setOutputPower(float dBm) {
  if ( !_esp_detected ) return;
  if ( _wifi != 255 ) {
    char buffer[10]; dtostrf(dBm,9,2,buffer); buffer[9] = '\0';
    uint8_t data[8+strlen(buffer)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x37; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(buffer); checksum ^= data[data_pos]; data_pos++; // dBm_LEN
    for ( uint32_t i = 0; i < strlen(buffer); i++ ) { data[data_pos] = buffer[i]; checksum ^= buffer[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::persistent(bool persistent) {
  if ( !_esp_detected ) return;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x38; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = persistent; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::mode(uint8_t m) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x39; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = m; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::getMode() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x40; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::enableSTA(bool enable) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x41; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = enable; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::enableAP(bool enable){
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x42; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = enable; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::forceSleepBegin(uint32_t sleepUs) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[11], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x43; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sleepUs >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sleepUs >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(sleepUs >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)sleepUs); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::forceSleepWake() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x44; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::endPacket() {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::parsePacket() {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return ((uint16_t)(storage_buffer[5] << 8) | storage_buffer[6]); }
  }
  return 0;
}
int teensquitto::beginPacket(const char *host, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[strlen(host) + 10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::beginPacket(IPAddress ip, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[13], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(ip)[0]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[1]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[2]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(ip)[3]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = port>>8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int teensquitto::beginPacketMulticast(IPAddress multicastAddress, uint16_t port, IPAddress interfaceAddress, int ttl) {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[21], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(multicastAddress)[0]; checksum ^= data[data_pos]; data_pos++; // multicastAddress
    data[data_pos] = IPAddress(multicastAddress)[1]; checksum ^= data[data_pos]; data_pos++; // multicastAddress
    data[data_pos] = IPAddress(multicastAddress)[2]; checksum ^= data[data_pos]; data_pos++; // multicastAddress
    data[data_pos] = IPAddress(multicastAddress)[3]; checksum ^= data[data_pos]; data_pos++; // multicastAddress
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++; // PORT >> 8
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++; // PORT
    data[data_pos] = IPAddress(interfaceAddress)[0]; checksum ^= data[data_pos]; data_pos++; // interfaceAddress
    data[data_pos] = IPAddress(interfaceAddress)[1]; checksum ^= data[data_pos]; data_pos++; // interfaceAddress
    data[data_pos] = IPAddress(interfaceAddress)[2]; checksum ^= data[data_pos]; data_pos++; // interfaceAddress
    data[data_pos] = IPAddress(interfaceAddress)[3]; checksum ^= data[data_pos]; data_pos++; // interfaceAddress
    data[data_pos] = ((uint8_t)(ttl >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(ttl >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(ttl >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)ttl); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port) {
  if ( !_esp_detected ) return 0;
  if ( _wifiudp != 255 ) {
    uint8_t data[17], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = IPAddress(multicast)[0]; checksum ^= data[data_pos]; data_pos++; // multicast
    data[data_pos] = IPAddress(multicast)[1]; checksum ^= data[data_pos]; data_pos++; // multicast
    data[data_pos] = IPAddress(multicast)[2]; checksum ^= data[data_pos]; data_pos++; // multicast
    data[data_pos] = IPAddress(multicast)[3]; checksum ^= data[data_pos]; data_pos++; // multicast
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++; // PORT >> 8
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++; // PORT
    data[data_pos] = IPAddress(interfaceAddr)[0]; checksum ^= data[data_pos]; data_pos++; // interfaceAddr
    data[data_pos] = IPAddress(interfaceAddr)[1]; checksum ^= data[data_pos]; data_pos++; // interfaceAddr
    data[data_pos] = IPAddress(interfaceAddr)[2]; checksum ^= data[data_pos]; data_pos++; // interfaceAddr
    data[data_pos] = IPAddress(interfaceAddr)[3]; checksum ^= data[data_pos]; data_pos++; // interfaceAddr
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::softAP(const char* ssid, const char* passphrase, int channel, int ssid_hidden, int max_connection) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    if ( passphrase == NULL ) { 
      uint8_t data[strlen(ssid)+strlen(passphrase)+8], checksum = 0; uint16_t data_pos = 0;
      data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
      data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
      data[data_pos] = 0x46; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
      for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
      data[data_pos] = checksum; // write crc
      deviceSerial->write(data, sizeof(data));
      uint32_t timeout = millis(); data_ready_flag = 0;
      while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
      if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
      if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
    }
    else { 
      uint8_t data[strlen(ssid)+strlen(passphrase)+12], checksum = 0; uint16_t data_pos = 0;
      data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
      data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
      data[data_pos] = 0x47; checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = strlen(passphrase); checksum ^= data[data_pos]; data_pos++;
      data[data_pos] = channel; checksum ^= data[data_pos]; data_pos++; // 0-13 min-max
      data[data_pos] = ssid_hidden; checksum ^= data[data_pos]; data_pos++; // 0 or 1
      data[data_pos] = max_connection; checksum ^= data[data_pos]; data_pos++; // 1-5?
      for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
      for ( uint32_t i = 0; i < strlen(passphrase); i++ ) { data[data_pos] = passphrase[i]; checksum ^= passphrase[i]; data_pos++; }
      data[data_pos] = checksum; // write crc
      deviceSerial->write(data, sizeof(data));
      uint32_t timeout = millis(); data_ready_flag = 0;
      while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
      if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
      if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
    }
  }
  return 0;
}
bool teensquitto::softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[19], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x48; checksum ^= data[data_pos]; data_pos++; // CONFIG
    data[data_pos] = IPAddress(local_ip)[0]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[1]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[2]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(local_ip)[3]; checksum ^= data[data_pos]; data_pos++; // LOCAL_IP
    data[data_pos] = IPAddress(gateway)[0]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[1]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[2]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(gateway)[3]; checksum ^= data[data_pos]; data_pos++; // GATEWAY
    data[data_pos] = IPAddress(subnet)[0]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[1]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[2]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = IPAddress(subnet)[3]; checksum ^= data[data_pos]; data_pos++; // SUBNET
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::softAPdisconnect(bool wifioff) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x49; checksum ^= data[data_pos]; data_pos++; // DISCONNECT
    data[data_pos] = wifioff; checksum ^= data[data_pos]; data_pos++; // OPTIONAL TURN WIFI OFF
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::softAPgetStationNum() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x50; checksum ^= data[data_pos]; data_pos++; // DISCONNECT
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
IPAddress teensquitto::softAPIP() {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x51; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
uint8_t* teensquitto::softAPmacAddress(uint8_t* mac) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x52; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return mac; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { mac[0] = storage_buffer[5]; mac[1] = storage_buffer[6]; mac[2] = storage_buffer[7]; mac[3] = storage_buffer[8]; mac[4] = storage_buffer[9]; mac[5] = storage_buffer[10]; return mac; }
  }
  return mac;
}
String teensquitto::softAPmacAddress(void) {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x53; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}
int8_t teensquitto::scanNetworks(bool async, bool show_hidden) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[9], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x54; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = async; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = show_hidden; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::scanNetworksAsync(asyncScanPtr handler, bool show_hidden) {
  if ( _wifi != 255 ) {
    wifi_Scan_hndl = handler;
    uint8_t data[8], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x55; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = show_hidden; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
}
int8_t teensquitto::scanComplete() {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x56; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return (int8_t)storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::scanDelete() {
  if ( !_esp_detected ) return;
  if ( _wifi != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x57; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::getNetworkInfo(uint8_t i, String &ssid, uint8_t &encType, int32_t &rssi, uint8_t* &bssid, int32_t &channel, bool &isHidden) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x58; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) {
      char buf[storage_buffer[6]+1]; for ( uint32_t i = 0; i < storage_buffer[6]; i++ ) { buf[i] = storage_buffer[23+i]; buf[i+1] = '\0'; } 
      ssid = buf; encType = storage_buffer[7];
      char _buf[4] = { storage_buffer[8], storage_buffer[9], storage_buffer[10], storage_buffer[11] }; memcpy(&rssi, _buf, 4);
      _mac_buffer[0] = storage_buffer[12]; _mac_buffer[1] = storage_buffer[13]; _mac_buffer[2] = storage_buffer[14];
      _mac_buffer[3] = storage_buffer[15]; _mac_buffer[4] = storage_buffer[16]; _mac_buffer[5] = storage_buffer[17];
      bssid = _mac_buffer;
      _buf[0] = storage_buffer[18]; _buf[1] = storage_buffer[19]; _buf[2] = storage_buffer[20]; _buf[3] = storage_buffer[21];
      memcpy(&channel, _buf, 4); isHidden = storage_buffer[22];
      return storage_buffer[5];
    }
  }
  return 0;
}
String teensquitto::SSID(uint8_t i) {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x59; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}
uint8_t teensquitto::encryptionType(uint8_t i) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x60; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
int32_t teensquitto::RSSI(uint8_t i) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x61; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[4] = { storage_buffer[5], storage_buffer[6], storage_buffer[7], storage_buffer[8] }; int32_t val; memcpy(&val, buf, 4); return val; }
  }
  return 0;
}
uint8_t * teensquitto::BSSID(uint8_t i) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x62; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { _mac_buffer[0] = storage_buffer[5]; _mac_buffer[1] = storage_buffer[6]; _mac_buffer[2] = storage_buffer[7]; _mac_buffer[3] = storage_buffer[8]; _mac_buffer[4] = storage_buffer[9]; _mac_buffer[5] = storage_buffer[10]; return _mac_buffer; }
  }
  return 0;
}
String teensquitto::BSSIDstr(uint8_t i) {
  if ( !_esp_detected ) return "";
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x63; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return ""; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[storage_buffer[5]+1]; for ( uint32_t i = 0; i < storage_buffer[5]; i++ ) { buf[i] = storage_buffer[6+i]; buf[i+1] = '\0'; } return String(buf); }
  }
  return "";
}
int32_t teensquitto::channel(uint8_t i) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x64; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { char buf[4] = { storage_buffer[5], storage_buffer[6], storage_buffer[7], storage_buffer[8] }; int32_t val; memcpy(&val, buf, 4); return val; }
  }
  return 0;
}
bool teensquitto::isHidden(uint8_t i) {
  if ( !_esp_detected ) return 0;
  if ( _wifi != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xA2; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xA2; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x65; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = i; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::commit() {
  if ( !_esp_detected ) return 0;
  if ( _esp_eeprom != 255 ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBB; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBB; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
uint8_t teensquitto::read(int const address) {
  if ( !_esp_detected ) return 0;
  if ( _esp_eeprom != 255 ) {
    uint8_t data[9], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBB; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBB; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = address >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = address; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::write(int const address, uint8_t const val) {
  if ( _esp_eeprom != 255 ) {
    uint8_t data[10], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBB; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBB; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = address >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = address; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = val; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::begin(uint32_t baudrate) { // set remote serial port and baud
  if ( !_esp_detected ) return;
  if ( _wifiudp != 255 ) {
    uint8_t data[9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xCC; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = baudrate >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = baudrate; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _esp_eeprom != 255 ) {
    uint8_t data[9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBB; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBB; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = baudrate >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = baudrate; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
size_t teensquitto::print(const char *p) {
  while (*p) {
    char c = *p++;
    write(c);
  }
  return 0;
}
size_t teensquitto::println(const char *p) {
  while (*p) {
    char c = *p++;
    write(c);
  }
  write('\n');
  return 0;
}
bool teensquitto::addAP(const char* ssid, const char *passphrase) {
  if ( !_esp_detected ) return 0;
  if ( _WiFiMulti ) {
    uint8_t data[strlen(ssid)+strlen(passphrase)+9], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(ssid); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(passphrase); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(ssid); i++ ) { data[data_pos] = ssid[i]; checksum ^= ssid[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(passphrase); i++ ) { data[data_pos] = passphrase[i]; checksum ^= passphrase[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5];}
  }
  return 0;
}
uint8_t teensquitto::run() {
  if ( !_esp_detected ) return 0;
  if ( _WiFiMulti ) {
    uint8_t data[7], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xBA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xBA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::onEvent(WSCPtr handler) {
  wsc_hndl = handler;
  uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x23; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; // write crc
  deviceSerial->write(data, sizeof(data));
}
void teensquitto::onEvent(WSSPtr handler) {
  wss_hndl = handler;
  uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x24; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; // write crc
  deviceSerial->write(data, sizeof(data));
}
void teensquitto::begin(String host, uint16_t port, String url, String protocol) { begin(host.c_str(), port, url.c_str(), protocol.c_str()); }
void teensquitto::begin(const char *host, uint16_t port, const char * url, const char * protocol) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[12+strlen(host)+strlen(url)+strlen(protocol)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(url); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(protocol); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(url); i++ ) { data[data_pos] = url[i]; checksum ^= url[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(protocol); i++ ) { data[data_pos] = protocol[i]; checksum ^= protocol[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::beginSSL(String host, uint16_t port, String url, String fingerprint, String protocol) { beginSSL(host.c_str(), port, url.c_str(), fingerprint.c_str(), protocol.c_str()); }
void teensquitto::beginSSL(const char *host, uint16_t port, const char * url, const char * fingerprint, const char * protocol) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[13+strlen(host)+strlen(url)+strlen(fingerprint)+strlen(protocol)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(url); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(fingerprint); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(protocol); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(url); i++ ) { data[data_pos] = url[i]; checksum ^= url[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(fingerprint); i++ ) { data[data_pos] = fingerprint[i]; checksum ^= fingerprint[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(protocol); i++ ) { data[data_pos] = protocol[i]; checksum ^= protocol[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::beginSocketIO(String host, uint16_t port, String url, String protocol) { beginSocketIO(host.c_str(), port, url.c_str(), protocol.c_str()); }
void teensquitto::beginSocketIO(const char *host, uint16_t port, const char * url, const char * protocol) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[12+strlen(host)+strlen(url)+strlen(protocol)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(url); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(protocol); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(url); i++ ) { data[data_pos] = url[i]; checksum ^= url[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(protocol); i++ ) { data[data_pos] = protocol[i]; checksum ^= protocol[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::beginSocketIOSSL(String host, uint16_t port, String url, String protocol) { beginSocketIOSSL(host.c_str(), port, url.c_str(), protocol.c_str()); }
void teensquitto::beginSocketIOSSL(const char *host, uint16_t port, const char * url, const char * protocol) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[12+strlen(host)+strlen(url)+strlen(protocol)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(host); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(url); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(protocol); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(host); i++ ) { data[data_pos] = host[i]; checksum ^= host[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(url); i++ ) { data[data_pos] = url[i]; checksum ^= url[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(protocol); i++ ) { data[data_pos] = protocol[i]; checksum ^= protocol[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::sendPing(String & payload) { return sendPing((uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::sendPing(uint8_t * payload, size_t length) {
  if ( !_esp_detected ) return 0;
  if ( _websocketclient != 255 ) {
    uint8_t data[11+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x05; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::setAuthorization(const char * user, const char * password) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[9+strlen(user)+strlen(password)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(user); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(password); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(user); i++ ) { data[data_pos] = user[i]; checksum ^= user[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(password); i++ ) { data[data_pos] = password[i]; checksum ^= password[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _websocketserver != 255 ) {
    uint8_t data[9+strlen(user)+strlen(password)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x20; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(user); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(password); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(user); i++ ) { data[data_pos] = user[i]; checksum ^= user[i]; data_pos++; }
    for ( uint32_t i = 0; i < strlen(password); i++ ) { data[data_pos] = password[i]; checksum ^= password[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::setAuthorization(const char * auth) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[8+strlen(auth)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(auth); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(auth); i++ ) { data[data_pos] = auth[i]; checksum ^= auth[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  if ( _websocketserver != 255 ) {
    uint8_t data[8+strlen(auth)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x21; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(auth); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(auth); i++ ) { data[data_pos] = auth[i]; checksum ^= auth[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::setExtraHeaders(const char * extraHeaders) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[8+strlen(extraHeaders)], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x08; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = strlen(extraHeaders); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < strlen(extraHeaders); i++ ) { data[data_pos] = extraHeaders[i]; checksum ^= extraHeaders[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
void teensquitto::setReconnectInterval(unsigned long time) {
  if ( !_esp_detected ) return;
  if ( _websocketclient != 255 ) {
    uint8_t data[11], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x09; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(time >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)time ); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::sendBIN(const uint8_t * payload, size_t length) { return sendBIN((uint8_t *) payload, length); }
bool teensquitto::sendBIN(uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketclient != 255 ) {
    uint8_t data[12+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x10; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::sendTXT(const uint8_t * payload, size_t length) { return sendTXT((uint8_t *) payload, length); }
bool teensquitto::sendTXT(char * payload, size_t length, bool headerToPayload) { return sendTXT((uint8_t *) payload, length, headerToPayload); }
bool teensquitto::sendTXT(const char * payload, size_t length) { return sendTXT((uint8_t *) payload, length); }
bool teensquitto::sendTXT(String & payload) { return sendTXT((uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::sendTXT(uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketclient != 255 ) {
    if( !length ) length = strlen((const char *) payload);
    uint8_t data[12+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x11; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::sendTXT(uint8_t num, const uint8_t * payload, size_t length) { return sendTXT(num, (uint8_t *) payload, length); }
bool teensquitto::sendTXT(uint8_t num, char * payload, size_t length, bool headerToPayload) { return sendTXT(num, (uint8_t *) payload, length, headerToPayload); }
bool teensquitto::sendTXT(uint8_t num, const char * payload, size_t length) { return sendTXT(num, (uint8_t *) payload, length); }
bool teensquitto::sendTXT(uint8_t num, String & payload) { return sendTXT(num, (uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::sendTXT(uint8_t num, uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    if( !length ) length = strlen((const char *) payload);
    uint8_t data[13+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = num; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::sendBIN(uint8_t num, const uint8_t * payload, size_t length) { return sendBIN(num, (uint8_t *) payload, length); }
bool teensquitto::sendBIN(uint8_t num, uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    uint8_t data[13+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = num; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::sendPing(uint8_t num, String & payload) { return sendPing(num, (uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::sendPing(uint8_t num, uint8_t * payload, size_t length) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    uint8_t data[12+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = num; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::broadcastTXT(const uint8_t * payload, size_t length) { return broadcastTXT((uint8_t *) payload, length); }
bool teensquitto::broadcastTXT(char * payload, size_t length, bool headerToPayload) { return broadcastTXT((uint8_t *) payload, length, headerToPayload); }
bool teensquitto::broadcastTXT(const char * payload, size_t length) { return broadcastTXT((uint8_t *) payload, length); }
bool teensquitto::broadcastTXT(String & payload) { return broadcastTXT((uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::broadcastTXT(uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    if( !length ) length = strlen((const char *) payload);
    uint8_t data[12+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::broadcastBIN(const uint8_t * payload, size_t length) { return broadcastBIN((uint8_t *) payload, length); }
bool teensquitto::broadcastBIN(uint8_t * payload, size_t length, bool headerToPayload) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    uint8_t data[12+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = headerToPayload; checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::broadcastPing(String & payload) { return broadcastPing((uint8_t *) payload.c_str(), payload.length()); }
bool teensquitto::broadcastPing(uint8_t * payload, size_t length) {
  if ( !_esp_detected ) return 0;
  if ( _websocketserver != 255 ) {
    uint8_t data[11+length], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(length >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)length); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < length; i++ ) { data[data_pos] = payload[i]; checksum ^= payload[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
IPAddress teensquitto::remoteIP(uint8_t num) {
  if ( !_esp_detected ) return IPAddress(0, 0, 0, 0);
  if ( _websocketserver != 255 ) {
    uint8_t data[8], data_pos = 0, checksum = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xEA; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xEA; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x22; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = num; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return IPAddress(0, 0, 0, 0); } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return IPAddress(storage_buffer[5],storage_buffer[6],storage_buffer[7],storage_buffer[8]); }
  }
  return IPAddress(0, 0, 0, 0);
}
bool teensquitto::setCACert(const uint8_t* pk, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11+size], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x12; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = pk[i]; checksum ^= pk[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setCertificate(const uint8_t* pk, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11+size], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x13; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = pk[i]; checksum ^= pk[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::setPrivateKey(const uint8_t* pk, size_t size) {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[11+size], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x14; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 24)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 16)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)(size >> 8)); checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = ((uint8_t)size); checksum ^= data[data_pos]; data_pos++;
    for ( uint32_t i = 0; i < size; i++ ) { data[data_pos] = pk[i]; checksum ^= pk[i]; data_pos++; }
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
void teensquitto::allowSelfSignedCerts() {
  if ( !_esp_detected ) return;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x15; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
  }
  return;
}
bool teensquitto::loadCACert() {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x16; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::loadCertificate() {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x17; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}
bool teensquitto::loadPrivateKey() {
  if ( !_esp_detected ) return 0;
  if ( _wificlientsecure != 255 ) {
    uint8_t data[7], checksum = 0; uint16_t data_pos = 0;
    data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
    data[data_pos] = 0xAD; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCC; checksum ^= data[data_pos]; data_pos++; // HEADER
    data[data_pos] = 0x18; checksum ^= data[data_pos]; data_pos++;
    data[data_pos] = checksum; // write crc
    deviceSerial->write(data, sizeof(data));
    uint32_t timeout = millis(); data_ready_flag = 0;
    while ( !data_ready_flag && millis() - timeout < _esp_latency + 1000UL ) { _pendingData = 1; events(); if ( data_ready_flag ) { if ( ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] != data[5] ) data_ready_flag = 0; else break; } } _pendingData = 0;
    if ( millis() - timeout > _esp_latency + 1000UL ) { return 0; } // timeout 
    if ( data_ready_flag && ((uint16_t)(storage_buffer[0] << 8) | storage_buffer[1]) == 0xAA55 && ((uint16_t)(storage_buffer[2] << 8) | storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && storage_buffer[4] == data[5] ) { return storage_buffer[5]; }
  }
  return 0;
}






































uint8_t teensquitto::events() {
  // ################################################
  // ## SYNC CONTROLLER FOR ESP DETECTION ###########
  // ################################################
  if ( !_esp_detected ) {
    if ( millis() - _esp_sync_interval > 250UL ) {
      _esp_sync_interval = millis(); uint8_t reply[6], checksum = 0; uint16_t data_pos = 0;
      reply[data_pos] = 0x24; data_pos++; reply[data_pos] = (sizeof(reply) - 3) >> 8; data_pos++; reply[data_pos] = sizeof(reply) - 3; data_pos++;
      reply[data_pos] = 0x7E; checksum ^= reply[data_pos]; data_pos++; reply[data_pos] = 0x7E; checksum ^= reply[data_pos]; data_pos++; // HEADER
      reply[data_pos] = checksum; deviceSerial->write(reply, sizeof(reply));
      _esp_check_latency = millis();
    }
  }
  if ( !_pendingData && _esp_detected && millis() - _esp_sync_timeout > 5000 ) { // timeout! force library offline
    _esp_detected = 0; if ( _debug_flag ) Serial.println(F("ESP not detected."));
  }

  // ################################################
  // ## UART DATA PROTOCOL ##########################
  // ################################################
  if ( deviceSerial->available() > 0 ) {
    switch ( deviceSerial->peek() ) {
      case '$': { // COMMAND PACKET
          uint32_t timeout = millis(); while ( millis() - timeout < 100 && deviceSerial->available() < 4 );
          deviceSerial->read(); uint16_t uartBufferPtr = 0, uartBufferSize = ((uint16_t)(deviceSerial->read() << 8) | deviceSerial->read());
          uint8_t uartBuffer[uartBufferSize], checksum = 0; timeout = millis();
          while ( millis() - timeout < 100 ) {
            if ( deviceSerial->available() > 0 ) {
              timeout = millis(); uartBuffer[uartBufferPtr] = deviceSerial->read();
              if ( uartBufferPtr < uartBufferSize - 1 ) checksum ^= uartBuffer[uartBufferPtr]; uartBufferPtr++;
            }
            if ( uartBufferPtr >= uartBufferSize ) {
              if ( uartBuffer[uartBufferSize - 1] != checksum ) {
                deviceSerial->print("!"); deviceSerial->write(0x15); deviceSerial->write((uint8_t)0x00); deviceSerial->write(uartBuffer[0]); deviceSerial->write(uartBuffer[1]); return '$';
              }
              processing_data(uartBuffer, uartBufferSize); return '$';
            }
          }
          deviceSerial->print("!"); deviceSerial->write(0x15); deviceSerial->write((uint8_t)0x00); deviceSerial->write(0xFF); deviceSerial->write(0xFF); return 0x15; // timeout
        }
      case '!': { // ACK / NAK RESPONSE FROM HOST
          deviceSerial->read(); uint8_t ack_nak_ptr = 0; uint32_t timeout = millis();
          while ( millis() - timeout < 100 ) {
            if ( deviceSerial->available() > 0 ) {
              timeout = millis(); ack_nak_buf[ack_nak_ptr] = deviceSerial->read(); ack_nak_ptr++;
            }
            if ( ack_nak_ptr >= 4 ) {
              return ack_nak_buf[0];
            }
          }
          deviceSerial->print("!"); deviceSerial->write(0x15); deviceSerial->write((uint8_t)0x00); deviceSerial->write(0xFF); deviceSerial->write(0xFF); return 0x15; // timeout
        }
      default: {
          deviceSerial->read(); break; // flush bad bytes
        }
    }
  }
  if ( !_pendingData && teensy_handler_queue.size() > 0 ) {
//      uint32_t _count = 0; for ( uint8_t i : teensy_handler_queue.front() ) _count++;
//      uint8_t buf[_count];
//Serial.print("             DeQueing Handler!  Queue size: ");
//Serial.println(teensy_handler_queue.size() );
     
    uint32_t _count = 0; for ( uint8_t i : teensy_handler_queue.front() ) _count++;
    uint8_t *_queue_pointer = &teensy_handler_queue.front()[0];
  //  Serial.print("PROCESS?: "); for ( uint8_t i = 0; i < _count; i++ ) { Serial.print(_queue_pointer[i]); Serial.print(" "); } Serial.println();
    processing_data(_queue_pointer, _count); teensy_handler_queue.pop_front();
  }
  return 0;
}

void teensquitto::processing_data(uint8_t *data, uint32_t len) {
  // ################################################
  // ## VALID PACKET, RESET TIMEOUT, ONLINE! ########
  // ################################################
  _esp_sync_timeout = millis(); if ( !_esp_detected || ((uint16_t)(data[0] << 8) | data[1]) == 0x8080 ) { _esp_detected = 1; delay(1000); if ( _espDetectHandler != nullptr ) _espDetectHandler(); if ( _debug_flag ) Serial.println(F("ESP detected.")); }


  // ################################################
  // ## VALID DATA RECEIVED, BEGIN PROCESSING #######
  // ################################################
  switch ( ((uint16_t)(data[0] << 8) | data[1]) ) {

    // ################################################
    // ## RESPONSES FROM ENDPOINTS ####################
    // ################################################
    case 0xAA55: { // DATA REQUEST RESPONSES
 //  Serial.print("DEC:   "); for ( uint16_t i = 0; i < len - 1; i++ ) { Serial.print(data[i],HEX); Serial.print(" "); } Serial.println();
       for ( uint16_t i = 0; i < len - 1; i++ ) { storage_buffer[i] = data[i]; storage_buffer[i+1] = '\0'; } data_ready_flag = 1; return; // dont send back ACK
     }

    // ################################################
    // ## WIFI STATUS ( wl_status_t ) #################
    // ################################################
    case 0x7272: { // WIFI STATUS ( wl_status_t )
        _wifi_status = data[2];
        if ( _debug_flag ) {
          switch ( _wifi_status ) {
            case 0x00: { Serial.print(F("*** WiFi Status 0: ")); Serial.println(F("WL_IDLE_STATUS ***")); break; }
            case 0x01: { Serial.print(F("*** WiFi Status 1: ")); Serial.println(F("WL_NO_SSID_AVAIL ***")); break; }
            case 0x02: { Serial.print(F("*** WiFi Status 2: ")); Serial.println(F("WL_SCAN_COMPLETED ***")); break; }
            case 0x03: { Serial.print(F("*** WiFi Status 3: ")); Serial.println(F("WL_CONNECTED ***")); break; }
            case 0x04: { Serial.print(F("*** WiFi Status 4: ")); Serial.println(F("WL_CONNECT_FAILED ***")); break; }
            case 0x05: { Serial.print(F("*** WiFi Status 5: ")); Serial.println(F("WL_CONNECTION_LOST ***")); break; }
            case 0x06: { Serial.print(F("*** WiFi Status 6: ")); Serial.println(F("WL_DISCONNECTED ***")); break; }
          }
        }
        if ( _wifi_status == 6 ) { _mqtt_online_flag = 0; _client_online_flag = 0; Serial.println(F("*** MQTT disconnected ***")); }
        return;
      }

    // ################################################
    // ## MQTT ONLINE STATUS (bool) ###################
    // ################################################
    case 0x7474: { // MQTT ONLINE STATUS ( bool )
        _mqtt_online_flag = data[2]; if ( _debug_flag ) ( data[2] ) ? Serial.println(F("*** MQTT connected ***")) : Serial.println(F("*** MQTT disconnected ***")); return;
        }

    // ################################################
    // ## MQTT CLIENT ONLINE STATUS ( bool ) ##########
    // ################################################
    case 0x7575: {
        _client_online_flag = data[2];
        if ( _debug_flag ) { Serial.print(F("*** MQTT client '")); Serial.print(local_clientId); ( data[2] ) ? Serial.println(F("' connected ***")) : Serial.println(F("' disconnected ***")); }
        // _mqtt_server_latency = (uint32_t)(data[3] << 24) | data[4] << 16 | data[5] << 8 | data[6];
        return;
      }

    // ################################################
    // ## MQTT SERVER SYNC STATUS ( bool ) ############
    // ################################################
    case 0x7676: {
        _mqtt_server_sync = data[2]; if ( _debug_flag ) ( data[2] ) ? Serial.println(F("*** MQTT server in sync ***")) : Serial.println(F("*** MQTT server out of sync ***")); return;
        // _mqtt_server_latency = (uint32_t)(data[3] << 24) | data[4] << 16 | data[5] << 8 | data[6];
      }

    // ################################################
    // ## MQTT CLIENT SYNC STATUS ( bool ) ############
    // ################################################
    case 0x7777: {
        _mqtt_client_sync = data[2]; if ( _debug_flag ) ( data[2] ) ? Serial.println(F("*** MQTT client in sync ***")) : Serial.println(F("*** MQTT client out of sync ***")); return;
        // _mqtt_client_latency = (uint32_t)(data[3] << 24) | data[4] << 16 | data[5] << 8 | data[6];
      }

    // ################################################
    // ## ESP DETECTION: AUTO SYNC RESPONSE ###########
    // ################################################
    case 0x7F7F: { // ESP8266 auto sync response
        _esp_latency = (((millis() - _esp_check_latency) + _esp_latency)/2); return;
      }

    // ################################################
    // ## ESP DETECTION: REBOOT/RESET OCCURED #########
    // ################################################
    case 0x8080: { // ESP8266 was restarted.
        Serial.println(F("ESP was restarted."));
        return; // possibly used for handler in future
      }


    case 0xAFAF: { // ESP8266 MQTT Handlers.
        if ( _pendingData ) {
          std::vector<uint8_t> _vector (len); std::copy(data, data+len, _vector.begin());
          teensy_handler_queue.push_back(_vector); return;
        }
        switch ( data[2] ) {
          case 0x00: {
              if ( mqtt_connect != NULL ) mqtt_connect(data[3]);
              return;
            }
          case 0x01: {
              if ( mqtt_disconnect != NULL ) mqtt_disconnect((AsyncMqttClientDisconnectReason)data[3]);
              return;
            }
          case 0x02: {
              if ( mqtt_sub != NULL ) mqtt_sub(((uint16_t)(data[4] << 8) | data[5]),data[3]);
              return;
            }
          case 0x03: {
              if ( mqtt_unsub != NULL ) mqtt_unsub(((uint16_t)(data[3] << 8) | data[4]));
              return;
            }
          case 0x04: {
              if ( mqtt_pub != NULL ) mqtt_pub(((uint16_t)(data[3] << 8) | data[4]));
              return;
            }
          case 0x05: {
              uint16_t _topic_len = ((uint16_t)(data[3] << 8) | data[4]);
              char _topic[_topic_len + 1]; for ( uint16_t i = 0; i < _topic_len; i++ ) _topic[i] = data[20 + i]; _topic[_topic_len] = '\0';
              uint32_t _len = ((uint32_t)(data[8] << 24) | data[9] << 16 | data[10] << 8 | data[11]);
              uint32_t _index = ((uint32_t)(data[12] << 24) | data[13] << 16 | data[14] << 8 | data[15]);
              uint32_t _total = ((uint32_t)(data[16] << 24) | data[17] << 16 | data[18] << 8 | data[19]);
              char _payload[len + 1]; for ( uint32_t i = 0; i < len; i++ ) _payload[i] = data[20 + ((uint16_t)(data[3] << 8) | data[4]) + i];
              AsyncMqttClientMessageProperties properties;
              properties.qos = data[5]; properties.dup = data[6]; properties.retain = data[7];
              if ( mqtt_message != NULL ) mqtt_message(_topic, _payload, properties, _len, _index, _total);
              return;
            }
        }
        return;
      } 


    case 0xABAB: { // ESP8266 WIFI Handlers.
        if ( _pendingData ) {
          std::vector<uint8_t> _vector (len); std::copy(data, data+len, _vector.begin());
          teensy_handler_queue.push_back(_vector); return;
        }
        switch ( data[2] ) {
          case 0x00: {
              if ( wifi_onSTAgotIP != NULL ) {
                WiFiEventStationModeGotIP dst;
                dst.ip = IPAddress(data[3],data[4],data[5],data[6]);
                dst.gw = IPAddress(data[7],data[8],data[9],data[10]);
                dst.mask = IPAddress(data[11],data[12],data[13],data[14]);
                wifi_onSTAgotIP(dst);
              }
              return;
            }
          case 0x01: {
              if ( wifi_onSTADisconnect != NULL ) {
                WiFiEventStationModeDisconnected dst;
                char _ssid[data[3] + 1]; for ( uint32_t i = 0; i < data[3]; i++ ) _ssid[i] = data[11 + i]; _ssid[data[3]] = '\0';
                dst.ssid = String(reinterpret_cast<char*>(_ssid));
                dst.bssid[0] = data[4];
                dst.bssid[1] = data[5];
                dst.bssid[2] = data[6];
                dst.bssid[3] = data[7];
                dst.bssid[4] = data[8];
                dst.bssid[5] = data[9];
                dst.reason = static_cast<WiFiDisconnectReason>(data[10]);
                wifi_onSTADisconnect(dst);
              }
              return;
            }
          case 0x02: {
              if ( wifi_onSTAConnected != NULL ) {
                WiFiEventStationModeConnected dst;
                char _ssid[data[3] + 1]; for ( uint32_t i = 0; i < data[3]; i++ ) _ssid[i] = data[14 + i]; _ssid[data[3]] = '\0';
                dst.ssid = String(reinterpret_cast<char*>(_ssid));
                dst.bssid[0] = data[4];
                dst.bssid[1] = data[5];
                dst.bssid[2] = data[6];
                dst.bssid[3] = data[7];
                dst.bssid[4] = data[8];
                dst.bssid[5] = data[9];
                char buf[4] = { data[10], data[11], data[12], data[13] }; 
                memcpy(&dst.channel, buf, 4);
                wifi_onSTAConnected(dst);
              }
              return;
            }
          case 0x03: {
              if ( wifi_onSTAAuthModeChange != NULL ) {
                WiFiEventStationModeAuthModeChanged dst;
                dst.oldMode = data[3];
                dst.newMode = data[4];
                wifi_onSTAAuthModeChange(dst);
              }
              return;
            }
          case 0x04: {
              if ( wifi_onSTADHCPtimeout != NULL ) wifi_onSTADHCPtimeout();
              return;
            }
          case 0x05: {
              if ( wifi_onAPConnected != NULL ) {
                WiFiEventSoftAPModeStationConnected dst;
                dst.mac[0] = data[3];
                dst.mac[1] = data[4];
                dst.mac[2] = data[5];
                dst.mac[3] = data[6];
                dst.mac[4] = data[7];
                dst.mac[5] = data[8];
                dst.aid = data[9];
                wifi_onAPConnected(dst);
              }
              return;
            }
          case 0x06: {
              if ( wifi_onAPDisconnected != NULL ) {
                WiFiEventSoftAPModeStationDisconnected dst;
                dst.mac[0] = data[3];
                dst.mac[1] = data[4];
                dst.mac[2] = data[5];
                dst.mac[3] = data[6];
                dst.mac[4] = data[7];
                dst.mac[5] = data[8];
                dst.aid = data[9];
                wifi_onAPDisconnected(dst);
              }
              return;
            }
          case 0x07: {
              if ( wifi_onAPProbe != NULL ) {
                WiFiEventSoftAPModeProbeRequestReceived dst;
                dst.mac[0] = data[3];
                dst.mac[1] = data[4];
                dst.mac[2] = data[5];
                dst.mac[3] = data[6];
                dst.mac[4] = data[7];
                dst.mac[5] = data[8];
                char buf[4] = { data[9], data[10], data[11], data[12] }; 
                memcpy(&dst.rssi, buf, 4);
                wifi_onAPProbe(dst);
              }
              return;
            }
          case 0x08: {
              if ( wifi_Scan_hndl != NULL ) {
                uint32_t _networks = ((uint32_t)(data[3] << 24) | data[4] << 16 | data[5] << 8 | data[6]);
                wifi_Scan_hndl(_networks);
              }
              return;
            }
        }
        return;
      } 
    case 0xCFCF: { // ESP8266 ASYNC UDP HANDLER.
        if ( _pendingData ) {
          std::vector<uint8_t> _vector (len); std::copy(data, data+len, _vector.begin());
          teensy_handler_queue.push_back(_vector); return;
        }
        switch ( data[2] ) {
          case 0x00: {
              AsyncUDPPacket::_isBroadcast = data[3];
              AsyncUDPPacket::_isMulticast = data[4];
              AsyncUDPPacket::_remoteIP = IPAddress(data[5],data[6],data[7],data[8]);
              AsyncUDPPacket::_remotePort = ((uint16_t)(data[9] << 8) | data[10]);
              AsyncUDPPacket::_localIP = IPAddress(data[11],data[12],data[13],data[14]);
              AsyncUDPPacket::_localPort = ((uint16_t)(data[15] << 8) | data[16]);
              AsyncUDPPacket::_length = ((uint16_t)(data[17] << 8) | data[18]);
              for ( uint16_t i = 0; i < ((uint16_t)(data[17] << 8) | data[18]); i++ ) { AsyncUDPPacket::_data[i] = data[19+i]; }                
              udp.run_udp_handler(); return; // handler checked at other end.
            }
        }
        return;
      } 
    case 0xABCD: { // ESP8266 ASYNC WEBSOCKETS
        if ( _pendingData ) {
          std::vector<uint8_t> _vector (len); std::copy(data, data+len, _vector.begin());
          teensy_handler_queue.push_back(_vector); return;
        }
        switch ( data[2] ) {
          case 0x00: { // SERVER HANDLER
              if ( wss_hndl != NULL ) {
                uint32_t _len = ((uint32_t)(data[5] << 24) | data[6] << 16 | data[7] << 8 | data[8]);
                uint8_t _data[_len]; for ( uint32_t i = 0; i < _len; i++ ) _data[i] = data[9+i];
                wss_hndl(data[3], (WStype_t)data[4], _data, _len);
              }
              return; // handler checked at other end.
            }
          case 0x01: { // CLIENT HANDLER ASYNC WEBSOCKETS
              if ( wsc_hndl != NULL ) {
                uint32_t _len = ((uint32_t)(data[4] << 24) | data[5] << 16 | data[6] << 8 | data[7]);
                uint8_t _data[_len]; for ( uint32_t i = 0; i < _len; i++ ) _data[i] = data[8+i];
                wsc_hndl((WStype_t)data[3], _data, _len);
              }
              return; // handler checked at other end.
            }
        }
        return;
      } 





    case 0x7373: { // MQTT ONLINE STATUS & RESUBSCRIBE STATUS
        _mqtt_online_flag = data[2];
        if ( _debug_flag ) {
          if ( _mqtt_online_flag ) { Serial.print(F("MQTT connected")); } else { Serial.println(F("MQTT disconnected")); return; }
          if ( ((uint16_t)(data[3] << 8) | data[4]) ) {  Serial.print(F(", PacketID: ")); Serial.print(((uint16_t)(data[3] << 8) | data[4]));  Serial.print(F(", '")); Serial.print(F("resubscribed to ")); for ( uint32_t i = 0; i < data[5]; i++ ) Serial.print((char)data[6 + i]); Serial.print(F("'")); } Serial.println();
        }
        return;
      } 


  }
 // Serial.print("HEX:   "); for ( uint16_t i = 0; i < len - 1; i++ ) { Serial.print(data[i],HEX); Serial.print(" "); } Serial.println();
  Serial.print("DEC:   "); for ( uint16_t i = 0; i < len - 1; i++ ) { Serial.print(data[i]); Serial.print(" "); } Serial.println();
 // Serial.print("CHAR:  "); for ( uint16_t i = 0; i < len - 1; i++ ) { Serial.print((char)data[i]); } Serial.println();

}
int8_t teensquitto::WFAN(uint8_t remote, uint16_t val, uint16_t timeout_ms) {
  uint32_t timeout = millis();
  while ( millis() - timeout < timeout_ms ) {
    uint8_t event = events();
    if ( event == 0x06 && ack_nak_buf[1] == remote && ((uint16_t)(ack_nak_buf[2] << 8) | ack_nak_buf[3]) == val ) return 1;
    if ( event == 0x15 && ack_nak_buf[1] == remote && ((uint16_t)(ack_nak_buf[2] << 8) | ack_nak_buf[3]) == val ) return 0;
  }
  return -1;
}
teensquitto WiFi = teensquitto("WiFi");
teensquitto SPIFFS = teensquitto("SPIFFS");
teensquitto ESP = teensquitto("ESP");
teensquitto WiFiMulti = teensquitto("WiFiMulti");
namespace std {
void __attribute__((weak)) __throw_bad_alloc() {
  Serial.println("Unable to allocate memory");
}
void __attribute__((weak)) __throw_length_error( char const*e ) {
  Serial.print("Length Error :"); Serial.println(e);
}
}

