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


#ifndef _teensquitto_H_
#define _teensquitto_H_

#include "Stream.h"
#include <SPI.h>
#include <Wire.h>
#include <FastLED.h>
#include <IPAddress.h>
#include <functional>
#include <ESPAsyncUDP.h>
#include <deque>
#include <queue>

#define WakeMode RFMode
typedef enum {
     FM_QIO = 0x00,
     FM_QOUT = 0x01,
     FM_DIO = 0x02,
     FM_DOUT = 0x03,
     FM_UNKNOWN = 0xff
} FlashMode_t;
enum RFMode {
    RF_DEFAULT = 0, // RF_CAL or not after deep-sleep wake up, depends on init data byte 108.
    RF_CAL = 1,      // RF_CAL after deep-sleep wake up, there will be large current.
    RF_NO_CAL = 2,   // no RF_CAL after deep-sleep wake up, there will only be small current.
    RF_DISABLED = 4 // disable RF after deep-sleep wake up, just like modem sleep, there will be the smallest current.
};
typedef enum {
    WDTO_0MS    = 0,   //!< WDTO_0MS
    WDTO_15MS   = 15,  //!< WDTO_15MS
    WDTO_30MS   = 30,  //!< WDTO_30MS
    WDTO_60MS   = 60,  //!< WDTO_60MS
    WDTO_120MS  = 120, //!< WDTO_120MS
    WDTO_250MS  = 250, //!< WDTO_250MS
    WDTO_500MS  = 500, //!< WDTO_500MS
    WDTO_1S     = 1000,//!< WDTO_1S
    WDTO_2S     = 2000,//!< WDTO_2S
    WDTO_4S     = 4000,//!< WDTO_4S
    WDTO_8S     = 8000 //!< WDTO_8S
} WDTO_t;

typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;

enum OpenMode {
    OM_DEFAULT = 0,
    OM_CREATE = 1,
    OM_APPEND = 2,
    OM_TRUNCATE = 4
};

enum AccessMode {
    AM_READ = 1,
    AM_WRITE = 2,
    AM_RW = AM_READ | AM_WRITE
};

typedef enum WiFiMode {
    WIFI_OFF = 0,
    WIFI_STA = 1,
    WIFI_AP = 2,
    WIFI_AP_STA = 3
} WiFiMode_t;

typedef enum WiFiPhyMode {
    WIFI_PHY_MODE_11B = 1,
    WIFI_PHY_MODE_11G = 2,
    WIFI_PHY_MODE_11N = 3
} WiFiPhyMode_t;

typedef enum WiFiSleepType {
    WIFI_NONE_SLEEP = 0,
    WIFI_LIGHT_SLEEP = 1,
    WIFI_MODEM_SLEEP = 2
} WiFiSleepType_t;

enum wl_enc_type {
  ENC_TYPE_WEP  = 5,
  ENC_TYPE_TKIP = 2,
  ENC_TYPE_CCMP = 4,
  ENC_TYPE_NONE = 7,
  ENC_TYPE_AUTO = 8
};

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};

enum class AsyncMqttClientDisconnectReason : int8_t {
  TCP_DISCONNECTED = 0,
  MQTT_UNACCEPTABLE_PROTOCOL_VERSION = 1,
  MQTT_IDENTIFIER_REJECTED = 2,
  MQTT_SERVER_UNAVAILABLE = 3,
  MQTT_MALFORMED_CREDENTIALS = 4,
  MQTT_NOT_AUTHORIZED = 5,
  ESP8266_NOT_ENOUGH_SPACE = 6,
  TLS_BAD_FINGERPRINT = 7
};
typedef enum WiFiEvent {
    WIFI_EVENT_STAMODE_CONNECTED = 0,
    WIFI_EVENT_STAMODE_DISCONNECTED,
    WIFI_EVENT_STAMODE_AUTHMODE_CHANGE,
    WIFI_EVENT_STAMODE_GOT_IP,
    WIFI_EVENT_STAMODE_DHCP_TIMEOUT,
    WIFI_EVENT_SOFTAPMODE_STACONNECTED,
    WIFI_EVENT_SOFTAPMODE_STADISCONNECTED,
    WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED,
    WIFI_EVENT_MAX,
    WIFI_EVENT_ANY = WIFI_EVENT_MAX,
    WIFI_EVENT_MODE_CHANGE
} WiFiEvent_t;

enum WiFiDisconnectReason {
    WIFI_DISCONNECT_REASON_UNSPECIFIED              = 1,
    WIFI_DISCONNECT_REASON_AUTH_EXPIRE              = 2,
    WIFI_DISCONNECT_REASON_AUTH_LEAVE               = 3,
    WIFI_DISCONNECT_REASON_ASSOC_EXPIRE             = 4,
    WIFI_DISCONNECT_REASON_ASSOC_TOOMANY            = 5,
    WIFI_DISCONNECT_REASON_NOT_AUTHED               = 6,
    WIFI_DISCONNECT_REASON_NOT_ASSOCED              = 7,
    WIFI_DISCONNECT_REASON_ASSOC_LEAVE              = 8,
    WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED         = 9,
    WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD      = 10,  /* 11h */
    WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD     = 11,  /* 11h */
    WIFI_DISCONNECT_REASON_IE_INVALID               = 13,  /* 11i */
    WIFI_DISCONNECT_REASON_MIC_FAILURE              = 14,  /* 11i */
    WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /* 11i */
    WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS       = 17,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID     = 18,  /* 11i */
    WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID  = 19,  /* 11i */
    WIFI_DISCONNECT_REASON_AKMP_INVALID             = 20,  /* 11i */
    WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION    = 21,  /* 11i */
    WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP       = 22,  /* 11i */
    WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED       = 23,  /* 11i */
    WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED    = 24,  /* 11i */
    WIFI_DISCONNECT_REASON_BEACON_TIMEOUT           = 200,
    WIFI_DISCONNECT_REASON_NO_AP_FOUND              = 201,
    WIFI_DISCONNECT_REASON_AUTH_FAIL                = 202,
    WIFI_DISCONNECT_REASON_ASSOC_FAIL               = 203,
    WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT        = 204,
};

struct WiFiEventModeChange {
    WiFiMode oldMode, newMode;
};

struct WiFiEventStationModeConnected {
    String ssid; uint8_t bssid[6], channel;
};

struct WiFiEventStationModeDisconnected {
    String ssid; uint8_t bssid[6]; WiFiDisconnectReason reason;
};

struct WiFiEventStationModeAuthModeChanged {
    uint8_t oldMode, newMode;
};

struct WiFiEventStationModeGotIP {
    IPAddress ip, mask, gw;
};

struct WiFiEventSoftAPModeStationConnected {
    uint8_t mac[6], aid;
};

struct WiFiEventSoftAPModeStationDisconnected {
    uint8_t mac[6], aid;
};

struct WiFiEventSoftAPModeProbeRequestReceived {
    int rssi; uint8_t mac[6];
};

struct AsyncMqttClientMessageProperties {
  uint8_t qos;
  bool dup;
  bool retain;
};

typedef enum {
    WStype_ERROR,
    WStype_DISCONNECTED,
    WStype_CONNECTED,
    WStype_TEXT,
    WStype_BIN,
	WStype_FRAGMENT_TEXT_START,
	WStype_FRAGMENT_BIN_START,
	WStype_FRAGMENT,
	WStype_FRAGMENT_FIN,
} WStype_t;

typedef void        (*mqttSubscribe)(uint16_t packetId, uint8_t qos); // onMqttSubscribe
typedef void        (*mqttUnsubscribe)(uint16_t packetId); // onMqttUnsubscribe
typedef void        (*mqttPublish)(uint16_t packetId); // onMqttPublish
typedef void        (*mqttDisconnect)(AsyncMqttClientDisconnectReason reason); // onMqttDisconnect
typedef void        (*mqttConnect)(bool sessionPresent); // onMqttConnect
typedef void        (*mqttMessage)(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total); // onMqttMessage

typedef void        (*onStationModeGotIPPtr)(const WiFiEventStationModeGotIP& event);
typedef void        (*onStationModeDisconnectedPtr)(const WiFiEventStationModeDisconnected& event);
typedef void        (*onStationModeConnectedPtr)(const WiFiEventStationModeConnected& event);
typedef void        (*onStationModeAuthModeChangedPtr)(const WiFiEventStationModeAuthModeChanged& event);
typedef void        (*onStationModeDHCPTimeoutPtr)();
typedef void        (*onSoftAPModeStationConnectedPtr)(const WiFiEventSoftAPModeStationConnected& event);
typedef void        (*onSoftAPModeStationDisconnectedPtr)(const WiFiEventSoftAPModeStationDisconnected& event);
typedef void        (*onSoftAPModeProbeRequestReceivedPtr)(const WiFiEventSoftAPModeProbeRequestReceived& event);

typedef void        (*asyncScanPtr)(int networksFound); // asyncScanPtr

typedef void        (*WSCPtr)(WStype_t type, uint8_t * payload, size_t length);
typedef void        (*WSSPtr)(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

typedef std::function<void(void)> espDetectPtr;

using namespace std;

class teensquitto : public Stream {

  public:
    teensquitto(const char *localName, Stream *UART, const char *connect_address, const char *mqttuser = NULL, const char *mqttpass = NULL);
    teensquitto(const char *remoteName, const char *remoteLocation, Stream *UART, const char *peripheral);
    teensquitto(const char *data, Stream *UART); 
    teensquitto(const char *data); 
    virtual uint8_t   events();
    virtual void      begin(uint32_t baudrate); // UART/UDP/EEPROM
    virtual int       read(); // SPIFFS//WIFICLIENT/UDP/WIFICLIENTSECURE
    virtual int       available(); // SPIFFS//WIFICLIENT/UDP/WIFICLIENTSECURE
    virtual size_t    write(uint8_t val) { return write(&val, 1); } // SPIFFS/WIFICLIENT/UDP
    virtual int       peek(); // SPIFFS//WIFICLIENT/UDP/WIFICLIENTSECURE
    virtual size_t    print(const char *p);
    virtual size_t    println(const char *p);
    virtual void      flush(); // SPIFFS/WIFICLIENT/UDP
    virtual uint8_t begin(const char* ssid, const char *passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true); // WiFi
    virtual uint8_t begin(char* ssid, char *passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true); // WiFi
    virtual int8_t    begin(); // I2C/SPI/WiFi/SPIFFS/AsyncWSS
    virtual bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = (uint32_t)0x00000000, IPAddress dns2 = (uint32_t)0x00000000); // WiFi
    virtual bool      disconnect(bool wifioff = false); // WiFi/MQTT/AsyncWSC/AsyncWSS
    virtual bool      reconnect(); // WiFi
    virtual bool      setAutoConnect(bool autoConnect); // WiFi
    virtual bool      getAutoConnect(); // WiFi
    virtual bool      setAutoReconnect(bool autoReconnect); // WiFi
    virtual bool      isConnected(); // WiFi
    virtual uint8_t   waitForConnectResult(); // WiFi
    virtual IPAddress localIP(); // WiFi/WIFICLIENT
    virtual String    macAddress(); // WiFi
    virtual String hostname(); // WiFi
    virtual bool hostname(char* aHostname); // WiFi
    virtual bool hostname(const char* aHostname); // WiFi
    virtual bool hostname(String aHostname); // WiFi
    virtual IPAddress subnetMask(); // WiFi
    virtual IPAddress gatewayIP(); // WiFi
    virtual IPAddress dnsIP(uint8_t dns_no = 0); // WiFi
    virtual String    SSID(); // WiFi
    virtual String    psk(); // WiFi
    virtual String    BSSIDstr(); // WiFi
    virtual int32_t   RSSI(); // WiFi
    virtual uint8_t   status(); // WiFi/WIFICLIENT
    virtual bool      open(const char* path, const char* mode); // SPIFFS
    virtual bool      open(const String& path, const char* mode); // SPIFFS
    virtual bool      format(); // SPIFFS
    virtual bool exists(const char* path); // SPIFFS
    virtual bool exists(const String& path); // SPIFFS
    virtual bool remove(const char* path); // SPIFFS
    virtual bool remove(const String& path); // SPIFFS
    virtual bool rename(const char* pathFrom, const char* pathTo); // SPIFFS
    virtual bool rename(const String& pathFrom, const String& pathTo); // SPIFFS
    virtual String readStringUntil(char terminator); // SPIFFS
    virtual bool      close(); // SPIFFS
    virtual size_t position(); // SPIFFS
    virtual size_t size(); // SPIFFS
    virtual String name(); // SPIFFS
    virtual size_t read(uint8_t* buf, size_t size); // SPIFFS/WIFICLIENTSECURE/WIFICLIENT/WIFIUDP
    virtual size_t readBytes(char *buffer, size_t length) { return read((uint8_t*)buffer, length); } // SPIFFS/WIFICLIENTSECURE/WIFICLIENT/WIFIUDP
    virtual bool seek(uint32_t pos, SeekMode mode); // SPIFFS
    virtual bool seek(uint32_t pos) { return seek(pos, SeekSet); } // SPIFFS
    virtual size_t write(const uint8_t *buf, size_t size); // SPIFFS/WIFICLIENT/WIFICLIENTSECURE
    virtual size_t write(const char *buffer, size_t size) { return write((const uint8_t *)buffer, size); } // SPIFFS
    virtual bool openDir(const char* path); // SPIFFS
    virtual bool openDir(const String& path) { return openDir(path.c_str()); } // SPIFFS
    virtual size_t fileSize(); // SPIFFS
    virtual bool next(); // SPIFFS
    virtual bool openFile(const char* mode); // SPIFFS
    virtual String fileName(); // SPIFFS
    virtual bool info(FSInfo& info); // SPIFFS
    virtual uint16_t subscribe(const char* topic, uint8_t qos); // MQTT
    virtual uint16_t unsubscribe(const char* topic); // MQTT
    virtual uint16_t publish(const char* topic, uint8_t qos, bool retain, const char* payload = nullptr, size_t length = 0, bool dup = false, uint16_t message_id = 0); // MQTT
    virtual bool connected(); // MQTT/WIFICLIENT/WIFICLIENTSECURE
    virtual bool connect(); // MQTT
    virtual void onConnect(mqttConnect handler); // MQTT
    virtual void onDisconnect(mqttDisconnect handler); // MQTT
    virtual void onSubscribe(mqttSubscribe handler); // MQTT
    virtual void onUnsubscribe(mqttUnsubscribe handler); // MQTT
    virtual void onMessage(mqttMessage handler); // MQTT
    virtual void onPublish(mqttPublish handler); // MQTT
    virtual bool setKeepAlive(uint16_t keepAlive); // MQTT
    virtual bool setClientId(const char* clientId); // MQTT
    virtual bool setCleanSession(bool cleanSession); // MQTT
    virtual bool setMaxTopicLength(uint16_t maxTopicLength); // MQTT
    virtual bool setCredentials(const char* username, const char* password = nullptr); // MQTT
    virtual bool setWill(const char* topic, uint8_t qos, bool retain, const char* payload = nullptr, size_t length = 0); // MQTT
    virtual bool setServer(IPAddress ip, uint16_t port); // MQTT
    virtual bool setServer(const char* host, uint16_t port); // MQTT
    virtual void onStationModeGotIP(onStationModeGotIPPtr handler); // WiFi
    virtual void onStationModeDisconnected(onStationModeDisconnectedPtr handler); // WiFi
    virtual void onStationModeConnected(onStationModeConnectedPtr handler); // WiFi
    virtual void onStationModeAuthModeChanged(onStationModeAuthModeChangedPtr handler); // WiFi
    virtual void onStationModeDHCPTimeout(onStationModeDHCPTimeoutPtr handler); // WiFi
    virtual void onSoftAPModeStationConnected(onSoftAPModeStationConnectedPtr handler); // WiFi
    virtual void onSoftAPModeStationDisconnected(onSoftAPModeStationDisconnectedPtr handler); // WiFi
    virtual void onSoftAPModeProbeRequestReceived(onSoftAPModeProbeRequestReceivedPtr handler); // WiFi
    virtual uint8_t * macAddress(uint8_t* mac); // WiFi
    virtual uint8_t * BSSID(); // WiFi                                                                          // TO BE IMPLEMENTED!
    virtual bool beginWPSConfig(void); // WiFi
    virtual bool beginSmartConfig(); // WiFi
    virtual bool stopSmartConfig(); // WiFi
    virtual bool smartConfigDone(); // WiFi
    virtual void reset(); // ESP
    virtual void restart(); // ESP
    virtual void wdtEnable(uint32_t timeout_ms = 0); // ESP
    virtual void wdtEnable(WDTO_t timeout_ms = WDTO_0MS); // ESP
    virtual void wdtDisable(); // ESP
    virtual void wdtFeed(); // ESP
    virtual void deepSleep(uint32_t time_us, RFMode mode = RF_DEFAULT); // ESP
    virtual bool rtcUserMemoryRead(uint32_t offset, uint32_t *rtcdata, size_t size); // ESP                            // TO BE IMPLEMENTED!
    virtual bool rtcUserMemoryWrite(uint32_t offset, uint32_t *rtcdata, size_t size); // ESP                           // TO BE IMPLEMENTED!
    virtual uint16_t getVcc(); // ESP
    virtual uint32_t getFreeHeap(); // ESP
    virtual uint32_t getChipId(); // ESP
    virtual uint8_t getBootVersion(); // ESP
    virtual uint8_t getBootMode(); // ESP
    virtual uint8_t getCpuFreqMHz(); // ESP
    virtual uint32_t getFlashChipId(); // ESP
    virtual const char * getSdkVersion(); // ESP
    virtual String getCoreVersion(); // ESP
    virtual uint32_t getFlashChipRealSize(); // ESP
    virtual uint32_t getFlashChipSize(); // ESP
    virtual uint32_t getFlashChipSpeed(); // ESP
    virtual uint8_t getFlashChipMode(); // ESP
    virtual uint32_t getFlashChipSizeByChipId(); // ESP
    virtual uint32_t magicFlashChipSize(uint8_t byte); // ESP
    virtual uint32_t magicFlashChipSpeed(uint8_t byte); // ESP
    virtual uint8_t magicFlashChipMode(uint8_t byte); // ESP
    virtual bool checkFlashConfig(bool needsEquals = false); // ESP
    virtual bool flashEraseSector(uint32_t sector); // ESP
    virtual bool flashWrite(uint32_t offset, uint32_t *data, size_t size); // ESP                           // TO BE IMPLEMENTED!
    virtual bool flashRead(uint32_t offset, uint32_t *data, size_t size); // ESP                           // TO BE IMPLEMENTED!
    virtual uint32_t getSketchSize(); // ESP
    virtual String getSketchMD5(); // ESP
    virtual uint32_t getFreeSketchSpace(); // ESP
    virtual bool updateSketch(Stream& in, uint32_t size, bool restartOnFail = false, bool restartOnSuccess = true); // ESP                           // TO BE IMPLEMENTED!
    virtual String getResetReason(); // ESP
    virtual String getResetInfo(); // ESP
    virtual struct rst_info * getResetInfoPtr(); // ESP                           // TO BE IMPLEMENTED!
    virtual bool eraseConfig(); // ESP
    virtual uint32_t getCycleCount(); // ESP
    virtual int connect(IPAddress ip, uint16_t port); // WIFICLIENT/WIFICLIENTSECURE
    virtual int connect(const char *host, uint16_t port); // WIFICLIENT/WIFICLIENTSECURE
    virtual int connect(const String host, uint16_t port); // WIFICLIENT/WIFICLIENTSECURE
    virtual size_t peekBytes(uint8_t *buffer, size_t length); // WIFICLIENT/WIFICLIENTSECURE
    virtual size_t peekBytes(char *buffer, size_t length) { return peekBytes((uint8_t *) buffer, length); } // WIFICLIENT/WIFICLIENTSECURE
    virtual bool stop(); // WIFICLIENT/UDP/WIFICLIENTSECURE
    virtual bool getNoDelay(); // WIFICLIENT
    virtual bool setNoDelay(bool nodelay); // WIFICLIENT
    virtual uint16_t  remotePort(); // WIFICLIENT/UDP
    virtual uint16_t  localPort(); // WIFICLIENT/UDP
    virtual void stopAll(); // WIFICLIENT/UDP
    virtual IPAddress remoteIP(); // WIFICLIENT/UDP
    virtual void setLocalPortStart(uint16_t port); // WIFICLIENT
    virtual int availableForWrite(); // WIFICLIENT
    virtual int hostByName(const char* aHostname, IPAddress& aResult); // WiFi
    virtual int hostByName(const char* aHostname, IPAddress& aResult, uint32_t timeout_ms); // WiFi
    virtual bool getPersistent(); // WiFi
    virtual int32_t channel(void); // WiFi
    virtual bool setSleepMode(uint8_t type); // WiFi
    virtual uint8_t getSleepMode(); // WiFi
    virtual bool setPhyMode(uint8_t mode); // WiFi
    virtual uint8_t getPhyMode(); // WiFi
    virtual void setOutputPower(float dBm); // WiFi
    virtual void persistent(bool persistent); // WiFi
    virtual bool mode(uint8_t); // WiFi
    virtual uint8_t getMode(); // WiFi
    virtual bool enableSTA(bool enable); // WiFi
    virtual bool enableAP(bool enable); // WiFi
    virtual bool forceSleepBegin(uint32_t sleepUs = 0); // WiFi
    virtual bool forceSleepWake(); // WiFi
    virtual int endPacket(); // UDP
    virtual IPAddress destinationIP(); // UDP
    virtual int parsePacket(); // UDP
    virtual int beginPacket(IPAddress ip, uint16_t port); // UDP
    virtual int beginPacket(const char *host, uint16_t port); // UDP
    virtual int beginPacketMulticast(IPAddress multicastAddress, uint16_t port, IPAddress interfaceAddress, int ttl = 1); // UDP
    virtual uint8_t beginMulticast(IPAddress interfaceAddr, IPAddress multicast, uint16_t port); // UDP
    virtual bool softAP(const char* ssid, const char* passphrase = NULL, int channel = 1, int ssid_hidden = 0, int max_connection = 4); // WiFi
    virtual bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet); // WiFi
    virtual bool softAPdisconnect(bool wifioff = false); // WiFi
    virtual uint8_t softAPgetStationNum(); // WiFi
    virtual IPAddress softAPIP(); // WiFi
    virtual uint8_t* softAPmacAddress(uint8_t* mac); // WiFi
    virtual String softAPmacAddress(void); // WiFi
    virtual int8_t scanNetworks(bool async = false, bool show_hidden = false); // WiFi
    virtual int8_t scanComplete(); // WiFi
    virtual void scanDelete(); // WiFi
    virtual bool getNetworkInfo(uint8_t networkItem, String &ssid, uint8_t &encryptionType, int32_t &RSSI, uint8_t* &BSSID, int32_t &channel, bool &isHidden); // WiFi
    virtual String SSID(uint8_t networkItem); // WiFi
    virtual uint8_t encryptionType(uint8_t networkItem); // WiFi
    virtual int32_t RSSI(uint8_t networkItem); // WiFi
    virtual uint8_t * BSSID(uint8_t networkItem); // WiFi
    virtual String BSSIDstr(uint8_t networkItem); // WiFi
    virtual int32_t channel(uint8_t networkItem); // WiFi
    virtual bool isHidden(uint8_t networkItem); // WiFi
    virtual void scanNetworksAsync(asyncScanPtr handler, bool show_hidden = false); // WiFi
    virtual uint8_t read(int const address); // EEPROM
    virtual void write(int const address, uint8_t const val); // EEPROM
    virtual bool commit(); // EEPROM
    virtual bool addAP(const char* ssid, const char *passphrase); // WiFiMulti
    virtual uint8_t run(); // WiFiMulti
    virtual void onEvent(WSCPtr handler); // AsyncWSC
    virtual void onEvent(WSSPtr handler); // AsyncWSS
    virtual void begin(const char *host, uint16_t port, const char * url = "/", const char * protocol = "arduino"); // AsyncWSC
    virtual void begin(String host, uint16_t port, String url = "/", String protocol = "arduino"); // AsyncWSC
    virtual void beginSSL(const char *host, uint16_t port, const char * url = "/", const char * = "", const char * protocol = "arduino"); // AsyncWSC
    virtual void beginSSL(String host, uint16_t port, String url = "/", String fingerprint = "", String protocol = "arduino"); // AsyncWSC
    virtual void beginSocketIO(const char *host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino"); // AsyncWSC
    virtual void beginSocketIO(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino"); // AsyncWSC
    virtual void beginSocketIOSSL(const char *host, uint16_t port, const char * url = "/socket.io/?EIO=3", const char * protocol = "arduino"); // AsyncWSC
    virtual void beginSocketIOSSL(String host, uint16_t port, String url = "/socket.io/?EIO=3", String protocol = "arduino"); // AsyncWSC
    virtual bool sendPing(uint8_t * payload = NULL, size_t length = 0); // AsyncWSC
    virtual bool sendPing(String & payload); // AsyncWSC
    virtual void setAuthorization(const char * user, const char * password); // AsyncWSC/AsyncWSS
    virtual void setAuthorization(const char * auth); // AsyncWSC/AsyncWSS
    virtual void setExtraHeaders(const char * extraHeaders = NULL); // AsyncWSC
    virtual void setReconnectInterval(unsigned long time); // AsyncWSC
    virtual bool sendBIN(uint8_t * payload, size_t length, bool headerToPayload = false); // AsyncWSC
    virtual bool sendBIN(const uint8_t * payload, size_t length); // AsyncWSC
    virtual bool sendTXT(uint8_t * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSC
    virtual bool sendTXT(const uint8_t * payload, size_t length = 0); // AsyncWSC
    virtual bool sendTXT(char * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSC
    virtual bool sendTXT(const char * payload, size_t length = 0); // AsyncWSC
    virtual bool sendTXT(String & payload); // AsyncWSC
    virtual bool sendTXT(uint8_t num, uint8_t * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSS
    virtual bool sendTXT(uint8_t num, const uint8_t * payload, size_t length = 0); // AsyncWSS
    virtual bool sendTXT(uint8_t num, char * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSS
    virtual bool sendTXT(uint8_t num, const char * payload, size_t length = 0); // AsyncWSS
    virtual bool sendTXT(uint8_t num, String & payload); // AsyncWSS
    virtual bool sendBIN(uint8_t num, uint8_t * payload, size_t length, bool headerToPayload = false); // AsyncWSS
    virtual bool sendBIN(uint8_t num, const uint8_t * payload, size_t length); // AsyncWSS
    virtual bool sendPing(uint8_t num, uint8_t * payload = NULL, size_t length = 0); // AsyncWSS
    virtual bool sendPing(uint8_t num, String & payload); // AsyncWSS
    virtual bool broadcastTXT(uint8_t * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSS
    virtual bool broadcastTXT(const uint8_t * payload, size_t length = 0); // AsyncWSS
    virtual bool broadcastTXT(char * payload, size_t length = 0, bool headerToPayload = false); // AsyncWSS
    virtual bool broadcastTXT(const char * payload, size_t length = 0); // AsyncWSS
    virtual bool broadcastTXT(String & payload); // AsyncWSS
    virtual bool broadcastBIN(uint8_t * payload, size_t length, bool headerToPayload = false); // AsyncWSS
    virtual bool broadcastBIN(const uint8_t * payload, size_t length); // AsyncWSS
    virtual bool broadcastPing(uint8_t * payload = NULL, size_t length = 0); // AsyncWSS
    virtual bool broadcastPing(String & payload); // AsyncWSS
    virtual IPAddress remoteIP(uint8_t num); // AsyncWSS
    virtual void onDetect(espDetectPtr handler); // ESP Detection handler
    virtual bool verify(const char* fingerprint, const char* domain_name); //WIFICLIENTSECURE
    virtual bool verifyCertChain(const char* domain_name); //WIFICLIENTSECURE
    virtual bool setCACert(const uint8_t* pk, size_t size); //WIFICLIENTSECURE
    virtual bool setCertificate(const uint8_t* pk, size_t size); //WIFICLIENTSECURE
    virtual bool setPrivateKey(const uint8_t* pk, size_t size); //WIFICLIENTSECURE
    virtual void allowSelfSignedCerts(); // WIFICLIENTSECURE
    virtual bool loadCACert(); // WIFICLIENTSECURE
    virtual bool loadCertificate(); // WIFICLIENTSECURE
    virtual bool loadPrivateKey(); // WIFICLIENTSECURE

  private:
    static std::deque<std::vector<uint8_t>> teensy_handler_queue;

    // ASYNC WEBSOCKETS HANDLER
    static WSCPtr wsc_hndl; // client 
    static WSSPtr wss_hndl; // server

    // ASYNC WIFI SCAN HANDLER
    static asyncScanPtr wifi_Scan_hndl; 

    // WiFi HANDLERS
    static onStationModeGotIPPtr wifi_onSTAgotIP; 
    static onStationModeDisconnectedPtr wifi_onSTADisconnect; 
    static onStationModeConnectedPtr wifi_onSTAConnected;
    static onStationModeAuthModeChangedPtr wifi_onSTAAuthModeChange; 
    static onStationModeDHCPTimeoutPtr wifi_onSTADHCPtimeout; 
    static onSoftAPModeStationConnectedPtr wifi_onAPConnected; 
    static onSoftAPModeStationDisconnectedPtr wifi_onAPDisconnected; 
    static onSoftAPModeProbeRequestReceivedPtr wifi_onAPProbe;
    static uint8_t _wifi_handlers;
    static void update_wifi_handlers();

    // ASYNC MQTT HANDLERS
    static mqttConnect mqtt_connect; 
    static mqttDisconnect mqtt_disconnect; 
    static mqttSubscribe mqtt_sub; 
    static mqttUnsubscribe mqtt_unsub; 
    static mqttMessage mqtt_message; 
    static mqttPublish mqtt_pub; 
    static uint8_t _mqtt_handlers;
    static void update_mqtt_handlers();

    // ESP DETECTION HANDLER
    static espDetectPtr _espDetectHandler;
    void _onDetect(); // ESP Detection handler

    virtual void      processing_data(uint8_t *data, uint32_t len);
    virtual  int8_t    WFAN(uint8_t remote, uint16_t val, uint16_t timeout_ms); 
    static uint8_t   ack_nak_buf[4];
    uint8_t   mqtt_ip[4];
    uint16_t  mqtt_port;
    char      remote_location[33];
    char      local_clientId[33];
    char      remote_clientId[33];
    char      char_ip[16];
    char      mqtt_user[33];
    char      mqtt_pass[33];
    uint8_t   _esp = -1;
    uint8_t   _esp_eeprom = -1;
    uint8_t   _wifi = -1;
    uint8_t   _wifiudp = -1;
    uint8_t   _wificlient = -1;
    uint8_t   _wificlientsecure = -1;
    uint8_t   _wifimulti = -1;
    uint8_t   _websocketclient = -1;
    uint8_t   _websocketserver = -1;
    volatile uint8_t   _espserial = -1;
    volatile uint8_t   remote_spi_port = -1;
    volatile uint8_t   fastled_support = -1;
    volatile uint8_t   servo_support = -1;
    volatile uint8_t   eeprom_support = -1;
    volatile uint8_t   serial_port = -1;
    volatile uint8_t   wire_port = -1;
    volatile uint8_t   esp_control = -1;
    volatile uint8_t   esp_wire = -1;
    volatile uint8_t   esp_spi = -1;
    volatile uint8_t   teensy_control = -1;
    volatile uint8_t   user_control = -1;
    volatile uint8_t   control_wifi = -1;
    static  uint8_t   storage_buffer[512];
    static  uint8_t _mac_buffer[6];
    static  bool      data_ready_flag;
    static  bool      _debug_flag;
    bool      _spiffs;
    bool      _client_online_flag;
    bool      _WiFiMulti = 0;
    static volatile bool      _mqtt_online_flag;
    static  uint8_t   _wifi_status;
    bool      _mqtt_client_sync;
    bool      _mqtt_support;
    uint32_t   _mqtt_client_latency;
    static volatile bool      _mqtt_server_sync;
    static   bool      _esp_detected;
    static   volatile bool      _pendingData;
    static   uint32_t   _esp_sync_timeout;
    static   uint32_t   _esp_sync_interval;
    static volatile uint32_t   _mqtt_server_latency;
    static   uint16_t   _esp_latency;
    static   uint32_t   _esp_check_latency;
    static Stream      *deviceSerial;
    friend class AsyncUDP;
    friend class AsyncUDPMessage;
};
extern teensquitto WiFi;
extern teensquitto SPIFFS;
extern teensquitto ESP;
extern teensquitto AsyncUDPPlugin;
extern teensquitto WiFiMulti;
#endif
