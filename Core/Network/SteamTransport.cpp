#ifdef STEAM_NETWORKING

#include "stdafx.h"
#include "Network/SteamTransport.h"

// Stub implementation — all methods fail gracefully so the system falls back to RakNet.
// TODO: Implement using SteamNetworkingSockets API when Steam LAN is selected.

SteamTransport::SteamTransport() : m_active(false) {}
SteamTransport::~SteamTransport() { stop(); }

bool SteamTransport::listen(uint16_t, int) { return false; }
void SteamTransport::stop() { m_active = false; }
bool SteamTransport::connect(const char*, uint16_t) { return false; }
void SteamTransport::disconnect() { stop(); }
void SteamTransport::tick() {}
bool SteamTransport::send(ConnectionId, const uint8_t*, uint32_t, PacketPriority, PacketReliability) { return false; }
IncomingPacket* SteamTransport::receive() { return nullptr; }
void SteamTransport::deallocatePacket(IncomingPacket* p) { if (p) { delete[] p->data; delete p; } }
void SteamTransport::closeConnection(ConnectionId) {}
bool SteamTransport::pollNewConnection(ConnectionId&) { return false; }
bool SteamTransport::pollDisconnection(ConnectionId&) { return false; }
bool SteamTransport::isActive() const { return m_active; }
int SteamTransport::getConnectionCount() const { return 0; }

#endif // STEAM_NETWORKING
