#include "stdafx.h"
#include "Network/NetworkAdapter.h"
#include "Socket.h"

NetworkAdapter::NetworkAdapter(ITransport* transport)
    : m_transport(transport)
{
}

void NetworkAdapter::tick() {
    if (!m_transport) return;

    m_transport->tick();

    // Receive data and push to registered Socket queues
    IncomingPacket* pkt;
    while ((pkt = m_transport->receive()) != nullptr) {
        auto it = m_connections.find(pkt->sender.id);
        if (it != m_connections.end() && it->second.socket) {
            it->second.socket->pushDataToQueue(pkt->data, pkt->length, false);
        }
        m_transport->deallocatePacket(pkt);
    }
}

void NetworkAdapter::registerConnection(ConnectionId conn, Socket* socket, INetworkPlayer* player) {
    ConnectionState state;
    state.id = conn;
    state.socket = socket;
    state.player = player;
    m_connections[conn.id] = state;
}

void NetworkAdapter::removeConnection(ConnectionId conn) {
    m_connections.erase(conn.id);
}

void NetworkAdapter::sendData(ConnectionId conn, const uint8_t* data, uint32_t len, bool lowPriority) {
    auto priority = lowPriority ? PacketPriority::Low : PacketPriority::High;
    m_transport->send(conn, data, len, priority, PacketReliability::ReliableOrdered);
}

void NetworkAdapter::mapPlayer(INetworkPlayer* player, ConnectionId conn) {
    auto it = m_connections.find(conn.id);
    if (it != m_connections.end()) {
        it->second.player = player;
    }
}

ConnectionId NetworkAdapter::getConnectionForPlayer(INetworkPlayer* player) {
    for (auto& pair : m_connections) {
        if (pair.second.player == player)
            return pair.second.id;
    }
    return ConnectionId{};
}
