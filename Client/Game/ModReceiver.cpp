#include "stdafx.h"
#include "ModReceiver.h"
#include "ModLoader/ModLoader.h"
#include <cstdio>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <ShlObj.h>

ModReceiver& ModReceiver::instance() {
    static ModReceiver inst;
    return inst;
}

std::string ModReceiver::hashToHex(const uint8_t hash[32]) {
    char buf[65];
    for (int i = 0; i < 32; i++) snprintf(buf + i * 2, 3, "%02x", hash[i]);
    buf[64] = 0;
    return buf;
}

static bool createDirsRecursive(const std::string& path) {
    // Use SHCreateDirectoryExA for recursive directory creation
    int ret = SHCreateDirectoryExA(NULL, path.c_str(), NULL);
    return (ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS || ret == ERROR_FILE_EXISTS);
}

static bool fileExists(const std::string& path) {
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

void ModReceiver::init(const std::string& modsCache, const std::string& packsCache) {
    m_modsCache = modsCache;
    m_packsCache = packsCache;
    createDirsRecursive(modsCache);
    createDirsRecursive(packsCache);
}

bool ModReceiver::isInCache(const uint8_t hash[32], bool isMod) const {
    std::string hex = hashToHex(hash);
    std::string dir = isMod ? m_modsCache : m_packsCache;
    std::string path = dir + "\\" + hex + (isMod ? ".dll" : ".mcpack");
    return fileExists(path);
}

std::vector<ModTransfer::ModManifestEntry> ModReceiver::processModManifest(
    const ModTransfer::ModManifest& manifest, const std::string& serverAddress) {
    std::vector<ModTransfer::ModManifestEntry> needed;
    for (size_t i = 0; i < manifest.entries.size(); i++) {
        const auto& mod = manifest.entries[i];
        if (!isInCache(mod.fileHash, true)) {
            needed.push_back(mod);
        } else {
            printf("[ModRecv] Cached: %s v%s\n", mod.modId.c_str(), mod.version.c_str());
        }
    }
    for (size_t i = 0; i < needed.size(); i++) {
        const auto& mod = needed[i];
        std::string hex = hashToHex(mod.fileHash);
        FileDownload dl;
        memcpy(dl.fileHash, mod.fileHash, 32);
        dl.totalSize = mod.fileSize;
        dl.receivedBytes = 0;
        dl.complete = false;
        dl.isMod = true;
        m_downloads[hex] = dl;
    }
    return needed;
}

std::vector<ModTransfer::ResourceManifestEntry> ModReceiver::processResourceManifest(
    const ModTransfer::ResourceManifest& manifest) {
    std::vector<ModTransfer::ResourceManifestEntry> needed;
    for (size_t i = 0; i < manifest.entries.size(); i++) {
        const auto& pack = manifest.entries[i];
        if (!isInCache(pack.fileHash, false)) {
            needed.push_back(pack);
        } else {
            printf("[ModRecv] Cached pack: %s\n", pack.name.c_str());
        }
    }
    for (size_t i = 0; i < needed.size(); i++) {
        const auto& pack = needed[i];
        std::string hex = hashToHex(pack.fileHash);
        FileDownload dl;
        memcpy(dl.fileHash, pack.fileHash, 32);
        dl.totalSize = pack.fileSize;
        dl.receivedBytes = 0;
        dl.complete = false;
        dl.isMod = false;
        m_downloads[hex] = dl;
    }
    return needed;
}

void ModReceiver::beginKeyExchange(uint8_t outPublicKey[32]) {
    m_sessionKey = CryptoUtils::generateX25519KeyPair();
    memcpy(outPublicKey, m_sessionKey.publicKey, 32);
}

void ModReceiver::setServerX25519Key(const uint8_t serverKey[32]) {
    m_sharedSecret = CryptoUtils::x25519SharedSecret(m_sessionKey.secretKey, serverKey);
}

bool ModReceiver::processChunk(const ModTransfer::FileChunk& chunk) {
    std::string hex = hashToHex(chunk.fileHash);
    auto it = m_downloads.find(hex);
    if (it == m_downloads.end()) return false;

    FileDownload& dl = it->second;
    if (dl.data.empty()) dl.data.resize(chunk.totalSize);

    size_t remain = (size_t)(dl.totalSize - chunk.offset);
    size_t copyLen = chunk.data.size() < remain ? chunk.data.size() : remain;
    memcpy(dl.data.data() + chunk.offset, chunk.data.data(), copyLen);
    dl.receivedBytes += (uint32_t)copyLen;

    if (onStatusUpdate) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Downloading: %.8s... %.0f%%", hex.c_str(),
                100.0f * dl.receivedBytes / dl.totalSize);
        onStatusUpdate(msg);
    }

    if (dl.receivedBytes >= dl.totalSize) {
        dl.complete = true;
        printf("[ModRecv] Complete: %s (%u bytes)\n", hex.c_str(), dl.totalSize);
    }

    for (auto kit = m_downloads.begin(); kit != m_downloads.end(); ++kit) {
        if (!kit->second.complete) return false;
    }
    return true;
}

bool ModReceiver::verifyAndInstall(const ModTransfer::ModManifest& modManifest) {
    bool allOk = true;

    for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
        const std::string& hex = it->first;
        FileDownload& dl = it->second;

        if (!dl.complete) { allOk = false; continue; }

        if (dl.isMod) {
            // Mod data is encrypted: nonce(24) + MAC(16) + ciphertext
            if (dl.data.size() < 24 || m_sharedSecret.empty()) { allOk = false; continue; }
            uint8_t nonce[24];
            memcpy(nonce, dl.data.data(), 24);
            std::vector<uint8_t> decrypted = CryptoUtils::decrypt(
                dl.data.data() + 24, dl.data.size() - 24,
                m_sharedSecret.data(), nonce);
            if (decrypted.empty()) {
                printf("[ModRecv] Decryption failed: %s\n", hex.c_str());
                allOk = false; continue;
            }

            std::vector<uint8_t> hash = CryptoUtils::hash256(decrypted.data(), decrypted.size());
            if (memcmp(hash.data(), dl.fileHash, 32) != 0) {
                printf("[ModRecv] Hash mismatch: %s\n", hex.c_str());
                allOk = false; continue;
            }

            bool sigOk = false;
            for (size_t j = 0; j < modManifest.entries.size(); j++) {
                const auto& entry = modManifest.entries[j];
                if (memcmp(entry.fileHash, dl.fileHash, 32) == 0) {
                    sigOk = CryptoUtils::verify(decrypted.data(), decrypted.size(),
                                                 entry.signature, modManifest.serverPublicKey);
                    break;
                }
            }
            if (!sigOk) {
                printf("[ModRecv] Signature invalid: %s\n", hex.c_str());
                allOk = false; continue;
            }

            std::string cachePath = m_modsCache + "\\" + hex + ".dll";
            std::ofstream f(cachePath.c_str(), std::ios::binary);
            f.write(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
            f.close();
            ModLoader::Get().LoadMod(cachePath);
            printf("[ModRecv] Installed mod: %s\n", hex.c_str());
        } else {
            std::vector<uint8_t> hash = CryptoUtils::hash256(dl.data.data(), dl.data.size());
            if (memcmp(hash.data(), dl.fileHash, 32) != 0) {
                printf("[ModRecv] Pack hash mismatch: %s\n", hex.c_str());
                allOk = false; continue;
            }
            std::string cachePath = m_packsCache + "\\" + hex + ".mcpack";
            std::ofstream f(cachePath.c_str(), std::ios::binary);
            f.write(reinterpret_cast<const char*>(dl.data.data()), dl.data.size());
            f.close();
            printf("[ModRecv] Installed pack: %s\n", hex.c_str());
        }
    }
    m_downloads.clear();
    return allOk;
}

bool ModReceiver::isServerTrusted(const std::string& address, const uint8_t publicKey[32]) const {
    for (size_t i = 0; i < m_trustedServers.size(); i++) {
        const TrustedServer& ts = m_trustedServers[i];
        if (ts.address == address && memcmp(ts.publicKey, publicKey, 32) == 0) return true;
    }
    return false;
}

void ModReceiver::trustServer(const std::string& address, const uint8_t publicKey[32]) {
    m_trustedServers.erase(
        std::remove_if(m_trustedServers.begin(), m_trustedServers.end(),
            [&](const TrustedServer& ts) { return ts.address == address; }),
        m_trustedServers.end());
    TrustedServer ts;
    ts.address = address;
    memcpy(ts.publicKey, publicKey, 32);
    m_trustedServers.push_back(ts);
}

void ModReceiver::loadTrustedKeys(const std::string& path) {
    std::ifstream f(path.c_str());
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        TrustedServer ts;
        ts.address = line.substr(0, sp);
        std::vector<uint8_t> key = CryptoUtils::base64Decode(line.substr(sp + 1));
        if (key.size() == 32) {
            memcpy(ts.publicKey, key.data(), 32);
            m_trustedServers.push_back(ts);
        }
    }
}

void ModReceiver::saveTrustedKeys(const std::string& path) const {
    std::ofstream f(path.c_str());
    for (size_t i = 0; i < m_trustedServers.size(); i++) {
        const TrustedServer& ts = m_trustedServers[i];
        f << ts.address << " " << CryptoUtils::base64Encode(ts.publicKey, 32) << "\n";
    }
}
