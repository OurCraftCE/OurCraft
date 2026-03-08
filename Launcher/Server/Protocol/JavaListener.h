#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

// Java Edition protocol listener (TCP) — handshake, status ping, login, play
class JavaListener {
public:
    JavaListener();
    ~JavaListener();

    bool start(uint16_t port, int maxPlayers);
    void stop();
    void tick();

    void setMotd(const std::wstring& motd) { m_motd = motd; }
    void setMaxPlayers(int max) { m_maxPlayers = max; }
    void setOnlinePlayers(int count) { m_onlinePlayers = count; }
    void setOnlineMode(bool online) { m_onlineMode = online; }

    int getConnectionCount() const;

    std::vector<std::string> getPlayerNames() const;
    bool kickPlayer(const std::string& name, const std::string& reason);
    void broadcastChat(const std::string& message);
    void disconnectAll(const std::string& reason);

    static const int PROTOCOL_VERSION = 47; // Java 1.8.9
    static const char* VERSION_NAME;

private:
    enum class ClientState { Handshake, Status, Login, Play, Disconnected };

    struct JavaClient {
        SOCKET sock = INVALID_SOCKET;
        ClientState state = ClientState::Handshake;
        std::vector<uint8_t> recvBuf;
        std::string username;
        std::string uuid;
        uint64_t connectionId = 0;
        DWORD lastActivity = 0;
        DWORD lastKeepAlive = 0;
        int32_t keepAliveId = 0;
        bool compressed = false;
        bool authenticated = false;
        std::vector<uint8_t> verifyToken;
        std::vector<uint8_t> sharedSecret;

        bool inTransferPhase = false;
        bool transferComplete = false;
        uint8_t clientX25519Key[32] = {};
    };

    static DWORD WINAPI acceptThread(LPVOID param);
    void acceptLoop();
    void processClient(JavaClient& client);
    bool readPacket(JavaClient& client, int32_t& packetId, std::vector<uint8_t>& payload);

    void handleHandshake(JavaClient& client, const std::vector<uint8_t>& payload);
    void handleStatusRequest(JavaClient& client);
    void handleStatusPing(JavaClient& client, const std::vector<uint8_t>& payload);
    void handleLoginStart(JavaClient& client, const std::vector<uint8_t>& payload);
    void handleEncryptionResponse(JavaClient& client, const std::vector<uint8_t>& payload);

    void sendEncryptionRequest(JavaClient& client);
    bool verifyMojangSession(const std::string& username, const std::string& serverHash,
                             std::string& outUuid, std::string& outName);
    std::string computeServerHash(const std::string& serverId,
                                  const std::vector<uint8_t>& sharedSecret,
                                  const std::vector<uint8_t>& publicKey);
    void completeLogin(JavaClient& client, const std::string& uuid, const std::string& name);

    void sendPacket(JavaClient& client, int32_t packetId, const std::vector<uint8_t>& data);
    void sendRaw(JavaClient& client, const uint8_t* data, int len);
    void sendJoinGame(JavaClient& client);
    void sendSpawnPosition(JavaClient& client);
    void sendPlayerPositionAndLook(JavaClient& client);
    void sendDisconnect(JavaClient& client, const std::string& reason);
    void sendChatMessage(JavaClient& client, const std::string& jsonMsg);
    void sendKeepAlive(JavaClient& client);
    void handleKeepAliveResponse(JavaClient& client, const std::vector<uint8_t>& payload);

    void sendModManifest(JavaClient& client);
    void sendResourceManifest(JavaClient& client);
    void sendFileChunks(JavaClient& client, const uint8_t clientX25519Key[32]);
    void handleModTransferResponse(JavaClient& client, const std::vector<uint8_t>& payload);
    void sendTransferPacket(JavaClient& client, uint8_t subType,
                            const std::vector<uint8_t>& data);

    std::string buildStatusJson();
    std::string offlineUuid(const std::string& name);

    SOCKET m_listenSock = INVALID_SOCKET;
    HANDLE m_acceptThread = nullptr;
    volatile bool m_running = false;
    uint16_t m_port = 25565;
    int m_maxPlayers = 8;
    int m_onlinePlayers = 0;
    std::wstring m_motd = L"Ourcraft Server";
    bool m_onlineMode = false;
    uint64_t m_nextConnId = 1;

    void* m_rsaKey = nullptr;    // HCRYPTKEY
    void* m_cryptProv = nullptr; // HCRYPTPROV
    std::vector<uint8_t> m_publicKeyDer; // ASN.1 DER X.509 public key
    bool initCrypto();
    void cleanupCrypto();

    std::mutex m_clientsMtx;
    std::vector<JavaClient> m_clients;

    static const DWORD CLIENT_TIMEOUT_MS = 30000;
};
