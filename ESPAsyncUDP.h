#ifndef ESPASYNCUDP_H
#define ESPASYNCUDP_H

#include "IPAddress.h"
#include "Print.h"
#include <functional>
#include <teensquitto.h>

class AsyncUDP;
class AsyncUDPPacketClass;
class AsyncUDPMessage;

struct ip4_addr;
typedef struct ip4_addr ip_addr_t;

class AsyncUDPMessage : public Print {
protected:
    friend class teensquitto;
    uint8_t *_buffer;
    size_t _index;
    size_t _size;
public:
    AsyncUDPMessage(size_t size=1460);
    virtual ~AsyncUDPMessage();
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
    size_t space();
    uint8_t * data();
    size_t length();
    void flush();
    operator bool() { return _buffer != NULL; }
};

class AsyncUDPPacket : public Print {
protected:
    friend class teensquitto;
    AsyncUDP *_udp;
    static uint8_t _data[256];
    static uint16_t _length;
    static bool _isBroadcast;
    static bool _isMulticast;
    static uint16_t _localPort;
    static uint16_t _remotePort;
    static IPAddress _remoteIP;
    static IPAddress _localIP;
public:
    AsyncUDPPacket(AsyncUDP *udp);
    virtual ~AsyncUDPPacket();
    uint8_t * data();
    size_t length();
    bool isBroadcast();
    bool isMulticast();
    IPAddress localIP();
    uint16_t localPort();
    IPAddress remoteIP();
    uint16_t remotePort();
    size_t send(AsyncUDPMessage &message);
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
};

typedef std::function<void(AsyncUDPPacket& packet)> AuPacketHandlerFunction;
typedef std::function<void(void * arg, AsyncUDPPacket& packet)> AuPacketHandlerFunctionWithArg;

class AsyncUDP : public Print {
protected:
    friend class teensquitto;
    IPAddress _remoteIP;
    uint16_t _remotePort;
    uint16_t _localPort;
    bool _connected;
    AuPacketHandlerFunction _handler;

public:
    AsyncUDP();
    virtual ~AsyncUDP();
    void onPacket(AuPacketHandlerFunctionWithArg cb, void * arg=NULL);
    void onPacket(AuPacketHandlerFunction cb);
    bool listen(ip_addr_t *addr, uint16_t port);
    bool listen(const IPAddress addr, uint16_t port);
    bool listen(uint16_t port);
    bool listenMulticast(ip_addr_t *addr, uint16_t port, uint8_t ttl=1);
    bool listenMulticast(const IPAddress addr, uint16_t port, uint8_t ttl=1);
    bool connect(ip_addr_t *addr, uint16_t port);
    bool connect(const IPAddress addr, uint16_t port);
    void close();
    void run_udp_handler();
    size_t writeTo(const uint8_t *data, size_t len, ip_addr_t *addr, uint16_t port);
    size_t writeTo(const uint8_t *data, size_t len, const IPAddress addr, uint16_t port);
    size_t write(const uint8_t *data, size_t len);
    size_t write(uint8_t data);
    size_t broadcastTo(uint8_t *data, size_t len, uint16_t port);
    size_t broadcastTo(const char * data, uint16_t port);
    size_t broadcast(uint8_t *data, size_t len);
    size_t broadcast(const char * data);

    size_t sendTo(AsyncUDPMessage &message, ip_addr_t *addr, uint16_t port);
    size_t sendTo(AsyncUDPMessage &message, const IPAddress addr, uint16_t port);
    size_t send(AsyncUDPMessage &message);

    size_t broadcastTo(AsyncUDPMessage &message, uint16_t port);
    size_t broadcast(AsyncUDPMessage &message);

    bool connected();
    operator bool();
};
extern AsyncUDP udp;
#endif
