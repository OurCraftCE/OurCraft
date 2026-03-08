#pragma once
#include "../../Server/Protocol/ModTransfer.h"
#include "../../Server/CryptoUtils.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct FileDownload {
    uint8_t fileHash[32];
    uint32_t totalSize;
    uint32_t receivedBytes;
    std::vector<uint8_t> data;
    bool complete;
    bool isMod;
};

struct TrustedServer {
    std::string address;
    uint8_t publicKey[32];
};

class ModReceiver {
public:
    static ModReceiver& instance();

    void init(const std::string& modsCache, const std::string& packsCache);

    // Process manifests — returns entries that need downloading
    std::vector<ModTransfer::ModManifestEntry> processModManifest(
        const ModTransfer::ModManifest& manifest, const std::string& serverAddress);
    std::vector<ModTransfer::ResourceManifestEntry> processResourceManifest(
        const ModTransfer::ResourceManifest& manifest);

    // Key exchange
    void beginKeyExchange(uint8_t outPublicKey[32]);
    void setServerX25519Key(const uint8_t serverKey[32]);

    // Chunk handling — returns true when ALL files complete
    bool processChunk(const ModTransfer::FileChunk& chunk);

    // Verify and install all completed downloads
    bool verifyAndInstall(const ModTransfer::ModManifest& modManifest);

    // Trust management
    bool isServerTrusted(const std::string& address, const uint8_t publicKey[32]) const;
    void trustServer(const std::string& address, const uint8_t publicKey[32]);
    void loadTrustedKeys(const std::string& path);
    void saveTrustedKeys(const std::string& path) const;

    bool isInCache(const uint8_t hash[32], bool isMod) const;

    // Callbacks
    std::function<void(const std::string& msg)> onStatusUpdate;

private:
    ModReceiver() = default;

    std::string m_modsCache;
    std::string m_packsCache;

    CryptoUtils::X25519KeyPair m_sessionKey = {};
    std::vector<uint8_t> m_sharedSecret;

    std::unordered_map<std::string, FileDownload> m_downloads;
    std::vector<TrustedServer> m_trustedServers;

    static std::string hashToHex(const uint8_t hash[32]);
};
