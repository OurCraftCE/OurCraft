#pragma once
#include "ITransport.h"
#include <queue>
#include <unordered_map>
#include <cstdint>

namespace RakNet {
    class RakPeerInterface;
    struct SystemAddress;
}

class RakNetTransport : public ITransport {
public:
    RakNetTransport();
    ~RakNetTransport() override;

    bool listen(uint16_t port, int maxConnections) override;
    void stop() override;
    bool connect(const char* host, uint16_t port) override;
    void disconnect() override;
    void tick() override;
    bool send(ConnectionId conn, const uint8_t* data, uint32_t len,
              PacketPriority priority = PacketPriority::Medium,
              PacketReliability reliability = PacketReliability::ReliableOrdered) override;
    IncomingPacket* receive() override;
    void deallocatePacket(IncomingPacket* packet) override;
    void closeConnection(ConnectionId conn) override;
    bool pollNewConnection(ConnectionId& outConn) override;
    bool pollDisconnection(ConnectionId& outConn) override;
    bool isActive() const override;
    int getConnectionCount() const override;
    void setOfflinePingResponse(const char* data, unsigned int length);

private:
    ConnectionId addressToConnectionId(const RakNet::SystemAddress& addr);

    RakNet::RakPeerInterface* m_peer;
    bool m_isServer;
    uint64_t m_nextConnectionId;

    std::unordered_map<uint64_t, void*> m_idToAddress;   // value is new'd SystemAddress*
    std::unordered_map<uint64_t, uint64_t> m_addrHashToId;

    std::queue<ConnectionId> m_newConnections;
    std::queue<ConnectionId> m_disconnections;
};
