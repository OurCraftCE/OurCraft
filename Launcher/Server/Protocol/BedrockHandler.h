#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "ModTransfer.h"

class RakNetTransport; // forward-declare to avoid RakNet headers

// Bedrock Edition protocol handler — wraps RakNetTransport for MOTD and packet handling
class BedrockHandler {
public:
    BedrockHandler();
    ~BedrockHandler();

    void init(RakNetTransport* transport, uint16_t port);

    void updateMotd(const std::wstring& motd, int onlinePlayers, int maxPlayers,
                    const std::wstring& worldName, const std::wstring& gamemode);

    void tick();

    void setOnlineMode(bool online) { m_onlineMode = online; }

    void sendModManifest(uint64_t clientId);
    void sendResourceManifest(uint64_t clientId);
    void sendFileChunks(uint64_t clientId, const uint8_t clientX25519Key[32]);
    void handleTransferPacket(uint64_t clientId, const uint8_t* data, size_t len);

    static const int PROTOCOL_VERSION = 361; // Bedrock 1.12.x
    static const char* VERSION_NAME;

private:
    void sendGamePacket(uint64_t clientId, const uint8_t* data, uint32_t len);

    RakNetTransport* m_transport = nullptr;
    uint16_t m_port = 19132;
    uint64_t m_serverGuid = 0;
    bool m_onlineMode = false;

    std::string m_motdString; // cached unconnected-pong MOTD string
};
