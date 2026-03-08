#pragma once
#ifdef STEAM_NETWORKING

#include "ITransport.h"
#include <queue>
#include <mutex>

class SteamTransport : public ITransport {
public:
    SteamTransport();
    ~SteamTransport();

    bool listen(uint16_t port, int maxConnections) override;
    void stop() override;
    bool connect(const char* host, uint16_t port) override;
    void disconnect() override;
    void tick() override;
    bool send(ConnectionId conn, const uint8_t* data, uint32_t len,
              PacketPriority priority, PacketReliability reliability) override;
    IncomingPacket* receive() override;
    void deallocatePacket(IncomingPacket* packet) override;
    void closeConnection(ConnectionId conn) override;
    bool pollNewConnection(ConnectionId& outConn) override;
    bool pollDisconnection(ConnectionId& outConn) override;
    bool isActive() const override;
    int getConnectionCount() const override;

private:
    bool m_active;
    std::queue<ConnectionId> m_newConnections;
    std::queue<ConnectionId> m_disconnections;
    std::mutex m_eventMutex;
};

#endif // STEAM_NETWORKING
