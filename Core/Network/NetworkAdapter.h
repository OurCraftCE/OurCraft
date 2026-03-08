#pragma once
#include "ITransport.h"
#include <unordered_map>

class Socket;
class INetworkPlayer;

// Bridges ITransport to the existing Socket/Connection system.
// Pumps RakNet data into Socket queues (inbound) and provides
// sendData for Socket output streams (outbound).
// Connection/disconnection events are handled externally (PlatformNetworkManagerStub).
class NetworkAdapter {
public:
    NetworkAdapter(ITransport* transport);

    // Call every tick — pumps ITransport receive, feeds data to registered Sockets
    void tick();

    // Register a Socket for a ConnectionId (called after CreateSocket)
    void registerConnection(ConnectionId conn, Socket* socket, INetworkPlayer* player);

    // Remove a connection's state
    void removeConnection(ConnectionId conn);

    // Send data from a Socket to the network
    void sendData(ConnectionId conn, const uint8_t* data, uint32_t len, bool lowPriority);

    // Map INetworkPlayer to ConnectionId
    void mapPlayer(INetworkPlayer* player, ConnectionId conn);
    ConnectionId getConnectionForPlayer(INetworkPlayer* player);

    // Get the transport
    ITransport* getTransport() { return m_transport; }

private:
    ITransport* m_transport;

    struct ConnectionState {
        ConnectionId id;
        Socket* socket;
        INetworkPlayer* player;
    };
    std::unordered_map<uint64_t, ConnectionState> m_connections;
};
