#pragma once
#include <string>
#include <cstdint>

struct ServerConfig {
    uint16_t bedrockPort = 19132;
    uint16_t javaPort = 25565;
    int maxPlayers = 8;
    std::wstring motd = L"Ourcraft Server";
    std::wstring worldName = L"world";
    int viewDistance = 10;
    bool onlineMode = false;
    std::wstring gamemode = L"survival";
    std::wstring difficulty = L"normal";
    int64_t seed = 0;          // 0 = random seed
    bool findSeed = true;      // true = generate random if seed==0

    // Mod system
    std::wstring modSigningKey = L"mod-keys/private.key";
    std::wstring modPublicKey = L"";  // Base64-encoded, auto-generated if empty
    bool enableMods = true;

    // Resource Packs
    bool forceServerPacks = true;
    int resourcePackMaxSize = 50 * 1024 * 1024; // 50MB

    // Transfer
    int transferChunkSize = 65536;  // 64KB
    int maxConcurrentTransfers = 4;

    static ServerConfig load(const char* path);
    void save(const char* path) const;
};
