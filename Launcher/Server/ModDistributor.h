#pragma once
#include "Protocol/ModTransfer.h"
#include "CryptoUtils.h"
#include <string>
#include <vector>
#include <unordered_map>

struct DistributableFile {
    std::string filePath;
    std::vector<uint8_t> fileData;
    uint8_t fileHash[32];
    uint32_t fileSize;
};

struct DistributableMod : DistributableFile {
    std::string modId;
    std::string modName;
    std::string version;
    uint8_t signature[64];
};

struct DistributablePack : DistributableFile {
    std::string uuid;
    std::string name;
    std::string version;
};

class ModDistributor {
public:
    static ModDistributor& instance();

    bool init(const char* serverDir, const char* signingKeyPath);

    const ModTransfer::ModManifest& getModManifest() const { return m_modManifest; }
    const ModTransfer::ResourceManifest& getResourceManifest() const { return m_resourceManifest; }

    const DistributableFile* getFileByHash(const uint8_t hash[32]) const;

    // Encrypt mod data for a specific client (using their X25519 public key)
    std::vector<uint8_t> encryptForClient(const DistributableMod& mod,
                                           const uint8_t clientX25519Public[32]) const;

    const std::vector<uint8_t>& getPackData(const std::string& uuid) const;

    // Console commands
    bool generateKeyPair(const char* outputDir);
    bool signMod(const char* dllPath);

    bool hasMods() const { return !m_mods.empty(); }
    bool hasPacks() const { return !m_packs.empty(); }

    const std::vector<DistributableMod>& getMods() const { return m_mods; }
    const std::vector<DistributablePack>& getPacks() const { return m_packs; }

private:
    ModDistributor() = default;

    void scanMods(const char* modsDir);
    void scanResourcePacks(const char* packsDir);
    bool loadSigningKey(const char* keyPath);

    CryptoUtils::Ed25519KeyPair m_signingKey = {};
    CryptoUtils::X25519KeyPair  m_x25519Key = {};
    bool m_hasSigningKey = false;

    std::vector<DistributableMod>  m_mods;
    std::vector<DistributablePack> m_packs;

    std::unordered_map<std::string, size_t> m_modsByHash;
    std::unordered_map<std::string, size_t> m_packsByHash;
    std::unordered_map<std::string, size_t> m_packsByUuid;

    ModTransfer::ModManifest      m_modManifest = {};
    ModTransfer::ResourceManifest m_resourceManifest = {};

    static const std::vector<uint8_t> s_emptyData;
};
