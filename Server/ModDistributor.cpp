#include "ModDistributor.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

const std::vector<uint8_t> ModDistributor::s_emptyData;

ModDistributor& ModDistributor::instance() {
    static ModDistributor inst;
    return inst;
}

static std::string hashToHex(const uint8_t hash[32]) {
    char buf[65];
    for (int i = 0; i < 32; i++) snprintf(buf + i * 2, 3, "%02x", hash[i]);
    buf[64] = 0;
    return buf;
}

bool ModDistributor::init(const char* serverDir, const char* signingKeyPath) {
    std::string dir(serverDir);
    m_x25519Key = CryptoUtils::generateX25519KeyPair();

    std::string keyPath = dir + "/" + signingKeyPath;
    if (loadSigningKey(keyPath.c_str())) {
        printf("[ModDist] Signing key loaded from %s\n", keyPath.c_str());
    } else {
        printf("[ModDist] No signing key — mod signing disabled\n");
    }

    scanMods((dir + "/mods").c_str());
    scanResourcePacks((dir + "/resource_packs").c_str());

    memset(m_modManifest.serverPublicKey, 0, 32);
    if (m_hasSigningKey)
        memcpy(m_modManifest.serverPublicKey, m_signingKey.publicKey, 32);
    memcpy(m_modManifest.x25519PublicKey, m_x25519Key.publicKey, 32);
    for (const auto& mod : m_mods) {
        ModTransfer::ModManifestEntry entry = {};
        entry.modId = mod.modId;
        entry.modName = mod.modName;
        entry.version = mod.version;
        entry.fileSize = mod.fileSize;
        memcpy(entry.signature, mod.signature, 64);
        memcpy(entry.fileHash, mod.fileHash, 32);
        m_modManifest.entries.push_back(entry);
    }

    m_resourceManifest.forceServerPacks = true;
    for (const auto& pack : m_packs) {
        ModTransfer::ResourceManifestEntry entry = {};
        entry.uuid = pack.uuid;
        entry.name = pack.name;
        entry.version = pack.version;
        entry.fileSize = pack.fileSize;
        memcpy(entry.fileHash, pack.fileHash, 32);
        m_resourceManifest.entries.push_back(entry);
    }

    printf("[ModDist] Ready: %zu mods, %zu resource packs\n", m_mods.size(), m_packs.size());
    return true;
}

bool ModDistributor::loadSigningKey(const char* keyPath) {
    auto data = CryptoUtils::loadKeyFile(keyPath);
    if (data.size() != 64) return false;
    memcpy(m_signingKey.privateKey, data.data(), 64);
    memcpy(m_signingKey.publicKey, data.data() + 32, 32);
    m_hasSigningKey = true;
    return true;
}

void ModDistributor::scanMods(const char* modsDir) {
    if (!fs::exists(modsDir)) {
        fs::create_directories(modsDir);
        return;
    }
    for (const auto& entry : fs::directory_iterator(modsDir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".dll" && ext != ".so") continue;

        std::ifstream f(entry.path(), std::ios::binary | std::ios::ate);
        if (!f.is_open()) continue;
        size_t size = (size_t)f.tellg();
        f.seekg(0);

        DistributableMod mod = {};
        mod.filePath = entry.path().string();
        mod.fileData.resize(size);
        f.read(reinterpret_cast<char*>(mod.fileData.data()), size);
        mod.fileSize = (uint32_t)size;

        auto hash = CryptoUtils::hash256(mod.fileData.data(), mod.fileData.size());
        memcpy(mod.fileHash, hash.data(), 32);

        std::string sigPath = mod.filePath + ".sig";
        auto sigData = CryptoUtils::loadKeyFile(sigPath.c_str());
        memset(mod.signature, 0, 64);
        if (sigData.size() == 64) {
            memcpy(mod.signature, sigData.data(), 64);
        } else if (m_hasSigningKey) {
            auto sig = CryptoUtils::sign(mod.fileData.data(), mod.fileData.size(), m_signingKey.privateKey);
            memcpy(mod.signature, sig.data(), 64);
            CryptoUtils::saveKeyFile(sigPath.c_str(), sig.data(), 64);
            printf("[ModDist] Auto-signed: %s\n", mod.filePath.c_str());
        }

        mod.modId = entry.path().stem().string();
        mod.modName = mod.modId;
        mod.version = "1.0.0";

        m_modsByHash[hashToHex(mod.fileHash)] = m_mods.size();
        printf("[ModDist] Loaded mod: %s (%u bytes)\n", mod.modId.c_str(), mod.fileSize);
        m_mods.push_back(std::move(mod));
    }
}

void ModDistributor::scanResourcePacks(const char* packsDir) {
    if (!fs::exists(packsDir)) {
        fs::create_directories(packsDir);
        return;
    }
    for (const auto& entry : fs::directory_iterator(packsDir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".zip" && ext != ".mcpack") continue;

        std::ifstream f(entry.path(), std::ios::binary | std::ios::ate);
        if (!f.is_open()) continue;
        size_t size = (size_t)f.tellg();
        f.seekg(0);

        DistributablePack pack = {};
        pack.filePath = entry.path().string();
        pack.fileData.resize(size);
        f.read(reinterpret_cast<char*>(pack.fileData.data()), size);
        pack.fileSize = (uint32_t)size;

        auto hash = CryptoUtils::hash256(pack.fileData.data(), pack.fileData.size());
        memcpy(pack.fileHash, hash.data(), 32);

        pack.name = entry.path().stem().string();
        // UUID derived from file hash for stable identity
        char uuid[37];
        snprintf(uuid, sizeof(uuid), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                pack.fileHash[0], pack.fileHash[1], pack.fileHash[2], pack.fileHash[3],
                pack.fileHash[4], pack.fileHash[5], pack.fileHash[6], pack.fileHash[7],
                pack.fileHash[8], pack.fileHash[9], pack.fileHash[10], pack.fileHash[11],
                pack.fileHash[12], pack.fileHash[13], pack.fileHash[14], pack.fileHash[15]);
        pack.uuid = uuid;
        pack.version = "1.0.0";

        std::string hexHash = hashToHex(pack.fileHash);
        m_packsByHash[hexHash] = m_packs.size();
        m_packsByUuid[pack.uuid] = m_packs.size();
        printf("[ModDist] Loaded pack: %s (%u bytes)\n", pack.name.c_str(), pack.fileSize);
        m_packs.push_back(std::move(pack));
    }
}

const DistributableFile* ModDistributor::getFileByHash(const uint8_t hash[32]) const {
    std::string hex = hashToHex(hash);
    auto it = m_modsByHash.find(hex);
    if (it != m_modsByHash.end()) return &m_mods[it->second];
    auto it2 = m_packsByHash.find(hex);
    if (it2 != m_packsByHash.end()) return &m_packs[it2->second];
    return nullptr;
}

std::vector<uint8_t> ModDistributor::encryptForClient(const DistributableMod& mod,
                                                       const uint8_t clientX25519Public[32]) const {
    auto shared = CryptoUtils::x25519SharedSecret(m_x25519Key.secretKey, clientX25519Public);
    uint8_t nonce[24];
    CryptoUtils::randomBytes(nonce, 24);
    auto encrypted = CryptoUtils::encrypt(mod.fileData.data(), mod.fileData.size(), shared.data(), nonce);
    // nonce prepended so client can derive same shared key and decrypt
    std::vector<uint8_t> result;
    result.reserve(24 + encrypted.size());
    result.insert(result.end(), nonce, nonce + 24);
    result.insert(result.end(), encrypted.begin(), encrypted.end());
    return result;
}

const std::vector<uint8_t>& ModDistributor::getPackData(const std::string& uuid) const {
    auto it = m_packsByUuid.find(uuid);
    if (it != m_packsByUuid.end()) return m_packs[it->second].fileData;
    return s_emptyData;
}

bool ModDistributor::generateKeyPair(const char* outputDir) {
    auto kp = CryptoUtils::generateSigningKeyPair();
    fs::create_directories(outputDir);
    std::string privPath = std::string(outputDir) + "/private.key";
    std::string pubPath = std::string(outputDir) + "/public.key";
    if (!CryptoUtils::saveKeyFile(privPath.c_str(), kp.privateKey, 64)) {
        printf("[ModDist] Failed to save private key\n");
        return false;
    }
    if (!CryptoUtils::saveKeyFile(pubPath.c_str(), kp.publicKey, 32)) {
        printf("[ModDist] Failed to save public key\n");
        return false;
    }
    auto b64 = CryptoUtils::base64Encode(kp.publicKey, 32);
    printf("[ModDist] Key pair generated:\n");
    printf("  Private: %s\n", privPath.c_str());
    printf("  Public:  %s\n", pubPath.c_str());
    printf("  Base64:  %s\n", b64.c_str());
    printf("  Add to server.properties: mod-public-key=%s\n", b64.c_str());
    return true;
}

bool ModDistributor::signMod(const char* dllPath) {
    if (!m_hasSigningKey) {
        printf("[ModDist] No signing key loaded. Run 'mod-keygen' first.\n");
        return false;
    }
    std::ifstream f(dllPath, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        printf("[ModDist] Cannot open: %s\n", dllPath);
        return false;
    }
    size_t size = (size_t)f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    auto sig = CryptoUtils::sign(data.data(), data.size(), m_signingKey.privateKey);
    std::string sigPath = std::string(dllPath) + ".sig";
    if (CryptoUtils::saveKeyFile(sigPath.c_str(), sig.data(), 64)) {
        printf("[ModDist] Signed: %s -> %s\n", dllPath, sigPath.c_str());
        return true;
    }
    printf("[ModDist] Failed to write signature\n");
    return false;
}
