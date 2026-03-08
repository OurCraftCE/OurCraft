#pragma once
#include "TransportTypes.h"

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual bool listen(uint16_t port, int maxConnections) = 0;
    virtual void stop() = 0;
    virtual bool connect(const char* host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual void tick() = 0;
    virtual bool send(ConnectionId conn, const uint8_t* data, uint32_t len,
                      PacketPriority priority = PacketPriority::Medium,
                      PacketReliability reliability = PacketReliability::ReliableOrdered) = 0;
    virtual IncomingPacket* receive() = 0;
    virtual void deallocatePacket(IncomingPacket* packet) = 0;
    virtual void closeConnection(ConnectionId conn) = 0;
    virtual bool pollNewConnection(ConnectionId& outConn) = 0;
    virtual bool pollDisconnection(ConnectionId& outConn) = 0;
    virtual bool isActive() const = 0;
    virtual int getConnectionCount() const = 0;
};
