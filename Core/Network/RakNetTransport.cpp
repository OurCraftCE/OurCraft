// RakNet's PacketPriority.h defines unscoped enums (PacketPriority, PacketReliability)
// that collide with our scoped enum class types in TransportTypes.h.
// Strategy: block RakNet's PacketPriority.h via its include guard, include our header
// first (defining enum class PacketPriority/PacketReliability), then provide the
// RakNet enum constants that RakPeerInterface.h needs as inline constexpr ints
// cast to our enum class type won't work — so we define them as plain ints and
// let RakPeerInterface.h use them through implicit int conversion.

// Step 1: Block RakNet's PacketPriority.h
#define __PACKET_PRIORITY_H

// Step 2: Include our header (defines enum class PacketPriority, PacketReliability, etc.)
#include "Network/RakNetTransport.h"

// Step 3: RakNet headers expect PacketPriority/PacketReliability as types for function
// parameters. Since our enum class types share the same name, RakPeerInterface.h will
// use our enum class. The default parameter values (e.g. LOW_PRIORITY) need to be
// defined as values of our enum class.
#define IMMEDIATE_PRIORITY  PacketPriority::Immediate
#define HIGH_PRIORITY       PacketPriority::High
#define MEDIUM_PRIORITY     PacketPriority::Medium
#define LOW_PRIORITY        PacketPriority::Low

#define UNRELIABLE              PacketReliability::Unreliable
#define UNRELIABLE_SEQUENCED    PacketReliability::UnreliableSequenced
#define RELIABLE                PacketReliability::Reliable
#define RELIABLE_ORDERED        PacketReliability::ReliableOrdered
#define RELIABLE_SEQUENCED      PacketReliability::ReliableSequenced
#define UNRELIABLE_WITH_ACK_RECEIPT         ((PacketReliability)5)
#define RELIABLE_WITH_ACK_RECEIPT           ((PacketReliability)7)
#define RELIABLE_ORDERED_WITH_ACK_RECEIPT   ((PacketReliability)8)
#define NUMBER_OF_PRIORITIES    ((PacketPriority)4)
#define NUMBER_OF_RELIABILITIES ((PacketReliability)9)

// Step 4: Now include RakNet headers — PacketPriority.h will be skipped,
// and our macros provide the enum values RakPeerInterface.h expects.
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "BitStream.h"

// Clean up macros
#undef IMMEDIATE_PRIORITY
#undef HIGH_PRIORITY
#undef MEDIUM_PRIORITY
#undef LOW_PRIORITY
#undef UNRELIABLE
#undef UNRELIABLE_SEQUENCED
#undef RELIABLE
#undef RELIABLE_ORDERED
#undef RELIABLE_SEQUENCED
#undef UNRELIABLE_WITH_ACK_RECEIPT
#undef RELIABLE_WITH_ACK_RECEIPT
#undef RELIABLE_ORDERED_WITH_ACK_RECEIPT
#undef NUMBER_OF_PRIORITIES
#undef NUMBER_OF_RELIABILITIES

#include <cstring>

// Hash a SystemAddress for lookup
static uint64_t hashSystemAddress(const RakNet::SystemAddress& addr) {
    // Use the binary representation
    uint64_t h = 14695981039346656037ULL;
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&addr);
    for (size_t i = 0; i < sizeof(RakNet::SystemAddress); ++i) {
        h ^= bytes[i];
        h *= 1099511628211ULL;
    }
    return h;
}

RakNetTransport::RakNetTransport()
    : m_peer(nullptr)
    , m_isServer(false)
    , m_nextConnectionId(1)
{
}

RakNetTransport::~RakNetTransport() {
    stop();
}

ConnectionId RakNetTransport::addressToConnectionId(const RakNet::SystemAddress& addr) {
    uint64_t h = hashSystemAddress(addr);
    auto it = m_addrHashToId.find(h);
    if (it != m_addrHashToId.end()) {
        return ConnectionId{ it->second };
    }
    // New address — assign a sequential ID and store mapping
    uint64_t id = m_nextConnectionId++;
    m_addrHashToId[h] = id;
    // Store a heap-allocated copy of the SystemAddress
    auto* addrCopy = new RakNet::SystemAddress(addr);
    m_idToAddress[id] = addrCopy;
    return ConnectionId{ id };
}

bool RakNetTransport::listen(uint16_t port, int maxConnections) {
    if (m_peer) return false;

    m_peer = RakNet::RakPeerInterface::GetInstance();
    RakNet::SocketDescriptor sd(port, nullptr);
    RakNet::StartupResult result = m_peer->Startup(static_cast<unsigned int>(maxConnections), &sd, 1);
    if (result != RakNet::RAKNET_STARTED) {
        RakNet::RakPeerInterface::DestroyInstance(m_peer);
        m_peer = nullptr;
        return false;
    }
    m_peer->SetMaximumIncomingConnections(static_cast<unsigned short>(maxConnections));
    m_isServer = true;
    return true;
}

void RakNetTransport::stop() {
    if (!m_peer) return;
    m_peer->Shutdown(300);
    RakNet::RakPeerInterface::DestroyInstance(m_peer);
    m_peer = nullptr;
    m_isServer = false;

    // Clean up address mappings
    for (auto& pair : m_idToAddress) {
        delete static_cast<RakNet::SystemAddress*>(pair.second);
    }
    m_idToAddress.clear();
    m_addrHashToId.clear();
    m_nextConnectionId = 1;

    // Clear event queues
    while (!m_newConnections.empty()) m_newConnections.pop();
    while (!m_disconnections.empty()) m_disconnections.pop();
}

bool RakNetTransport::connect(const char* host, uint16_t port) {
    if (m_peer) return false;

    m_peer = RakNet::RakPeerInterface::GetInstance();
    RakNet::SocketDescriptor sd;
    RakNet::StartupResult result = m_peer->Startup(1, &sd, 1);
    if (result != RakNet::RAKNET_STARTED) {
        RakNet::RakPeerInterface::DestroyInstance(m_peer);
        m_peer = nullptr;
        return false;
    }
    RakNet::ConnectionAttemptResult car = m_peer->Connect(host, port, nullptr, 0);
    if (car != RakNet::CONNECTION_ATTEMPT_STARTED) {
        m_peer->Shutdown(0);
        RakNet::RakPeerInterface::DestroyInstance(m_peer);
        m_peer = nullptr;
        return false;
    }
    m_isServer = false;
    return true;
}

void RakNetTransport::disconnect() {
    stop();
}

void RakNetTransport::tick() {
    // RakNet has its own network thread; nothing to pump here.
    // Connection/disconnection events are processed in receive().
}

bool RakNetTransport::send(ConnectionId conn, const uint8_t* data, uint32_t len,
                           ::PacketPriority priority, ::PacketReliability reliability) {
    if (!m_peer || !conn.isValid()) return false;

    auto it = m_idToAddress.find(conn.id);
    if (it == m_idToAddress.end()) return false;
    const auto& addr = *static_cast<RakNet::SystemAddress*>(it->second);

    // Prepend user message ID byte
    RakNet::BitStream bs;
    bs.Write(static_cast<unsigned char>(ID_USER_PACKET_ENUM));
    bs.Write(reinterpret_cast<const char*>(data), len);

    // Our enum class values have the same integer mapping as RakNet's enums
    // (both start at 0 in the same order). The macro trick above made RakNet's
    // function signatures use our enum class types directly.
    m_peer->Send(&bs, priority, reliability, 0,
                 RakNet::AddressOrGUID(addr), false);
    return true;
}

IncomingPacket* RakNetTransport::receive() {
    if (!m_peer) return nullptr;

    for (;;) {
        RakNet::Packet* rkPacket = m_peer->Receive();
        if (!rkPacket) return nullptr;

        if (rkPacket->length == 0) {
            m_peer->DeallocatePacket(rkPacket);
            continue;
        }

        unsigned char msgId = rkPacket->data[0];

        switch (msgId) {
            case ID_NEW_INCOMING_CONNECTION: {
                ConnectionId cid = addressToConnectionId(rkPacket->systemAddress);
                m_newConnections.push(cid);
                m_peer->DeallocatePacket(rkPacket);
                continue;
            }
            case ID_CONNECTION_REQUEST_ACCEPTED: {
                ConnectionId cid = addressToConnectionId(rkPacket->systemAddress);
                m_newConnections.push(cid);
                m_peer->DeallocatePacket(rkPacket);
                continue;
            }
            case ID_DISCONNECTION_NOTIFICATION:
            case ID_CONNECTION_LOST: {
                ConnectionId cid = addressToConnectionId(rkPacket->systemAddress);
                m_disconnections.push(cid);
                m_peer->DeallocatePacket(rkPacket);
                continue;
            }
            default:
                break;
        }

        // Skip non-user messages
        if (msgId < ID_USER_PACKET_ENUM) {
            m_peer->DeallocatePacket(rkPacket);
            continue;
        }

        // User packet: strip the ID_USER_PACKET_ENUM byte
        ConnectionId sender = addressToConnectionId(rkPacket->systemAddress);
        uint32_t payloadLen = rkPacket->length - 1;

        auto* packet = new IncomingPacket();
        packet->sender = sender;
        packet->length = payloadLen;
        packet->data = new uint8_t[payloadLen];
        std::memcpy(packet->data, rkPacket->data + 1, payloadLen);

        m_peer->DeallocatePacket(rkPacket);
        return packet;
    }
}

void RakNetTransport::deallocatePacket(IncomingPacket* packet) {
    if (!packet) return;
    delete[] packet->data;
    delete packet;
}

void RakNetTransport::closeConnection(ConnectionId conn) {
    if (!m_peer || !conn.isValid()) return;

    auto it = m_idToAddress.find(conn.id);
    if (it == m_idToAddress.end()) return;
    const auto& addr = *static_cast<RakNet::SystemAddress*>(it->second);

    m_peer->CloseConnection(RakNet::AddressOrGUID(addr), true);
}

bool RakNetTransport::pollNewConnection(ConnectionId& outConn) {
    if (m_newConnections.empty()) return false;
    outConn = m_newConnections.front();
    m_newConnections.pop();
    return true;
}

bool RakNetTransport::pollDisconnection(ConnectionId& outConn) {
    if (m_disconnections.empty()) return false;
    outConn = m_disconnections.front();
    m_disconnections.pop();
    return true;
}

bool RakNetTransport::isActive() const {
    return m_peer != nullptr;
}

int RakNetTransport::getConnectionCount() const {
    if (!m_peer) return 0;
    unsigned short count = 0;
    m_peer->GetConnectionList(nullptr, &count);
    return static_cast<int>(count);
}

void RakNetTransport::setOfflinePingResponse(const char* data, unsigned int length) {
    if (m_peer)
        m_peer->SetOfflinePingResponse(data, length);
}
