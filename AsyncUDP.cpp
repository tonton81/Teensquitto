#include "Arduino.h"
#include "ESPAsyncUDP.h"
#include <teensquitto.h>

// updated by teensquitto
IPAddress AsyncUDPPacket::_remoteIP = IPAddress(0, 0, 0, 0);
IPAddress AsyncUDPPacket::_localIP = IPAddress(0, 0, 0, 0);
uint16_t AsyncUDPPacket::_localPort = 0;
uint16_t AsyncUDPPacket::_remotePort = 0;
uint16_t AsyncUDPPacket::_length = 0;
bool AsyncUDPPacket::_isMulticast = 0;
bool AsyncUDPPacket::_isBroadcast = 0;
uint8_t AsyncUDPPacket::_data[256] = { 0 };

AsyncUDPMessage::AsyncUDPMessage(size_t size) {
  _index = 0;
  if(size > 1460) size = 1460;
  _size = size;
  _buffer = (uint8_t *)malloc(size);
}
AsyncUDPMessage::~AsyncUDPMessage() { if(_buffer) free(_buffer); }
size_t AsyncUDPMessage::write(const uint8_t *data, size_t len) {
  if(_buffer == NULL) return 0;
  size_t s = space();
  if(len > s) len = s;
  memcpy(_buffer + _index, data, len);
  _index += len;
  return len;
}
size_t AsyncUDPMessage::write(uint8_t data) { return write(&data, 1); }
size_t AsyncUDPMessage::space() {
  if(_buffer == NULL) return 0;
  return _size - _index;
}
uint8_t * AsyncUDPMessage::data() { return _buffer; }
size_t AsyncUDPMessage::length() { return _index; }
void AsyncUDPMessage::flush() { _index = 0; }


AsyncUDPPacket::AsyncUDPPacket(AsyncUDP *udp) { }
AsyncUDPPacket::~AsyncUDPPacket() { }
uint8_t * AsyncUDPPacket::data() { return _data; }
size_t AsyncUDPPacket::length() { return _length; }
IPAddress AsyncUDPPacket::localIP() { return _localIP; }
uint16_t AsyncUDPPacket::localPort() { return _localPort; }
IPAddress AsyncUDPPacket::remoteIP() { return _remoteIP; }
uint16_t AsyncUDPPacket::remotePort() { return _remotePort; }
bool AsyncUDPPacket::isBroadcast() { return _isBroadcast; }
bool AsyncUDPPacket::isMulticast() { return _isMulticast; }
size_t AsyncUDPPacket::write(const uint8_t *data, size_t len) { return _udp->writeTo(data, len, _remoteIP, _remotePort); }
size_t AsyncUDPPacket::write(uint8_t data) { return write(&data, 1); }
size_t AsyncUDPPacket::send(AsyncUDPMessage &message) { return write(message.data(), message.length()); }

AsyncUDP::AsyncUDP() { _connected = false; _handler = NULL; }
AsyncUDP::~AsyncUDP() { close(); }
AsyncUDP::operator bool() { return connected(); }
bool AsyncUDP::connected() {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t data[7], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x06; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] != data[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] == data[5] ) { return teensquitto::storage_buffer[5]; }
  return 0;
}
void AsyncUDP::onPacket(AuPacketHandlerFunctionWithArg cb, void * arg) { onPacket(std::bind(cb, arg, std::placeholders::_1)); }
void AsyncUDP::onPacket(AuPacketHandlerFunction cb) {
  if ( !teensquitto::_esp_detected ) return;
  uint8_t data[7], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x07; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  _handler = cb;
}
bool AsyncUDP::listen(ip_addr_t *addr, uint16_t port) { return 0; }
bool AsyncUDP::listen(const IPAddress addr, uint16_t port) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t data[13], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x00; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[0]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[1]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[2]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[3]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] != data[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] == data[5] ) { return teensquitto::storage_buffer[5]; }
  return 0;
}
bool AsyncUDP::listen(uint16_t port) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t data[9], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x01; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] != data[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] == data[5] ) { return teensquitto::storage_buffer[5]; }
  return 0;
}
bool AsyncUDP::listenMulticast(ip_addr_t *addr, uint16_t port, uint8_t ttl) { return 0; }
bool AsyncUDP::connect(ip_addr_t *addr, uint16_t port) { return 0; }
void AsyncUDP::close() {
  if ( !teensquitto::_esp_detected ) return;
  uint8_t data[7], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x02; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
}
void AsyncUDP::run_udp_handler() { AsyncUDPPacket packet(this); if (_handler) _handler(packet); }
bool AsyncUDP::listenMulticast(const IPAddress addr, uint16_t port, uint8_t ttl) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t data[14], data_pos = 0, checksum = 0;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x04; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[0]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[1]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[2]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[3]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = ttl; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] != data[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] == data[5] ) { return teensquitto::storage_buffer[5]; }
  return 0;
}
bool AsyncUDP::connect(const IPAddress addr, uint16_t port) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t data[13], data_pos = 0, checksum = 0; _remoteIP = addr; _remotePort = port;
  data[data_pos] = 0x24; data_pos++; data[data_pos] = (sizeof(data) - 3) >> 8; data_pos++; data[data_pos] = sizeof(data) - 3; data_pos++;
  data[data_pos] = 0xCE; checksum = data[data_pos]; data_pos++; data[data_pos] = 0xCE; checksum ^= data[data_pos]; data_pos++; // HEADER
  data[data_pos] = 0x03; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[0]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[1]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[2]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = IPAddress(addr)[3]; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port >> 8; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = port; checksum ^= data[data_pos]; data_pos++;
  data[data_pos] = checksum; teensquitto::deviceSerial->write(data, sizeof(data));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] != data[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(data[3] << 8) | data[4]) && teensquitto::storage_buffer[4] == data[5] ) { return teensquitto::storage_buffer[5]; }
  return 0;
}
size_t AsyncUDP::writeTo(const uint8_t *data, size_t len, ip_addr_t *addr, uint16_t port) { return 0; }
size_t AsyncUDP::writeTo(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t datar[17+((uint32_t)len)], checksum = 0; uint16_t data_pos = 0;
  datar[data_pos] = 0x24; data_pos++; datar[data_pos] = (sizeof(datar) - 3) >> 8; data_pos++; datar[data_pos] = sizeof(datar) - 3; data_pos++;
  datar[data_pos] = 0xCE; checksum = datar[data_pos]; data_pos++; datar[data_pos] = 0xCE; checksum ^= datar[data_pos]; data_pos++; // HEADER
  datar[data_pos] = 0x05; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 24)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 16)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 8)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)len); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = IPAddress(addr)[0]; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = IPAddress(addr)[1]; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = IPAddress(addr)[2]; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = IPAddress(addr)[3]; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = port >> 8; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = port; checksum ^= datar[data_pos]; data_pos++;
  for ( uint32_t i = 0; i < len; i++ ) { datar[data_pos] = data[i]; checksum ^= datar[data_pos]; data_pos++; }
  datar[data_pos] = checksum; teensquitto::deviceSerial->write(datar, sizeof(datar));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] != datar[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] == datar[5] ) { return ((uint32_t)(teensquitto::storage_buffer[5] << 24) | teensquitto::storage_buffer[6] << 16 | teensquitto::storage_buffer[7] << 8 | teensquitto::storage_buffer[8]); }
  return 0;
}
size_t AsyncUDP::write(uint8_t data) { return write(&data, 1); }
size_t AsyncUDP::write(const uint8_t *data, size_t len) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t datar[11+((uint32_t)len)], checksum = 0; uint16_t data_pos = 0;
  datar[data_pos] = 0x24; data_pos++; datar[data_pos] = (sizeof(datar) - 3) >> 8; data_pos++; datar[data_pos] = sizeof(datar) - 3; data_pos++;
  datar[data_pos] = 0xCE; checksum = datar[data_pos]; data_pos++; datar[data_pos] = 0xCE; checksum ^= datar[data_pos]; data_pos++; // HEADER
  datar[data_pos] = 0x08; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 24)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 16)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 8)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)len); checksum ^= datar[data_pos]; data_pos++;
  for ( uint32_t i = 0; i < len; i++ ) { datar[data_pos] = data[i]; checksum ^= datar[data_pos]; data_pos++; }
  datar[data_pos] = checksum; teensquitto::deviceSerial->write(datar, sizeof(datar));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] != datar[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] == datar[5] ) { return ((uint32_t)(teensquitto::storage_buffer[5] << 24) | teensquitto::storage_buffer[6] << 16 | teensquitto::storage_buffer[7] << 8 | teensquitto::storage_buffer[8]); }
  return 0;
}
size_t AsyncUDP::broadcastTo(const char * data, uint16_t port) { return broadcastTo((uint8_t *)data, strlen(data), port); }
size_t AsyncUDP::broadcastTo(uint8_t *data, size_t len, uint16_t port) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t datar[13+((uint32_t)len)], checksum = 0; uint16_t data_pos = 0;
  datar[data_pos] = 0x24; data_pos++; datar[data_pos] = (sizeof(datar) - 3) >> 8; data_pos++; datar[data_pos] = sizeof(datar) - 3; data_pos++;
  datar[data_pos] = 0xCE; checksum = datar[data_pos]; data_pos++; datar[data_pos] = 0xCE; checksum ^= datar[data_pos]; data_pos++; // HEADER
  datar[data_pos] = 0x10; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 24)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 16)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 8)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)len); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = port >> 8; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = port; checksum ^= datar[data_pos]; data_pos++;
  for ( uint32_t i = 0; i < len; i++ ) { datar[data_pos] = data[i]; checksum ^= datar[data_pos]; data_pos++; }
  datar[data_pos] = checksum; teensquitto::deviceSerial->write(datar, sizeof(datar));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] != datar[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] == datar[5] ) { return ((uint32_t)(teensquitto::storage_buffer[5] << 24) | teensquitto::storage_buffer[6] << 16 | teensquitto::storage_buffer[7] << 8 | teensquitto::storage_buffer[8]); }
  return 0;
}
size_t AsyncUDP::broadcast(const char * data) { return broadcast((uint8_t *)data, strlen(data)); }
size_t AsyncUDP::broadcast(uint8_t *data, size_t len) {
  if ( !teensquitto::_esp_detected ) return 0;
  uint8_t datar[11+((uint32_t)len)], checksum = 0; uint16_t data_pos = 0;
  datar[data_pos] = 0x24; data_pos++; datar[data_pos] = (sizeof(datar) - 3) >> 8; data_pos++; datar[data_pos] = sizeof(datar) - 3; data_pos++;
  datar[data_pos] = 0xCE; checksum = datar[data_pos]; data_pos++; datar[data_pos] = 0xCE; checksum ^= datar[data_pos]; data_pos++; // HEADER
  datar[data_pos] = 0x09; checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 24)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 16)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)(len >> 8)); checksum ^= datar[data_pos]; data_pos++;
  datar[data_pos] = ((uint8_t)len); checksum ^= datar[data_pos]; data_pos++;
  for ( uint32_t i = 0; i < len; i++ ) { datar[data_pos] = data[i]; checksum ^= datar[data_pos]; data_pos++; }
  datar[data_pos] = checksum; teensquitto::deviceSerial->write(datar, sizeof(datar));
  uint32_t timeout = millis(); teensquitto::data_ready_flag = 0;
  while ( !teensquitto::data_ready_flag && millis() - timeout < teensquitto::_esp_latency + 500UL ) { teensquitto::_pendingData = 1; WiFi.events(); if ( teensquitto::data_ready_flag ) { if ( ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) != ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] != datar[5] ) teensquitto::data_ready_flag = 0; else break; } } teensquitto::_pendingData = 0;
  if ( millis() - timeout > teensquitto::_esp_latency + 500UL ) { return 0; } // timeout 
  if ( teensquitto::data_ready_flag && ((uint16_t)(teensquitto::storage_buffer[0] << 8) | teensquitto::storage_buffer[1]) == 0xAA55 && ((uint16_t)(teensquitto::storage_buffer[2] << 8) | teensquitto::storage_buffer[3]) == ((uint16_t)(datar[3] << 8) | datar[4]) && teensquitto::storage_buffer[4] == datar[5] ) { return ((uint32_t)(teensquitto::storage_buffer[5] << 24) | teensquitto::storage_buffer[6] << 16 | teensquitto::storage_buffer[7] << 8 | teensquitto::storage_buffer[8]); }
  return 0;
}
size_t AsyncUDP::sendTo(AsyncUDPMessage &message, ip_addr_t *addr, uint16_t port) { return 0; }
size_t AsyncUDP::sendTo(AsyncUDPMessage &message, const IPAddress addr, uint16_t port) {
  if(!message) return 0; return writeTo(message.data(), message.length(), addr, port);
}
size_t AsyncUDP::send(AsyncUDPMessage &message) {
  if(!message) return 0; return write(message.data(), message.length());
}
size_t AsyncUDP::broadcastTo(AsyncUDPMessage &message, uint16_t port) {
  if(!message) return 0; return broadcastTo(message.data(), message.length(), port);
}
size_t AsyncUDP::broadcast(AsyncUDPMessage &message) {
 if(!message) return 0; return broadcast(message.data(), message.length());
}

AsyncUDP udp;