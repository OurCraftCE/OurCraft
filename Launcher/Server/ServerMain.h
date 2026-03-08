#pragma once
#include <cstdint>

struct ServerInitParams {
    uint16_t bedrockPort = 19132;
    uint16_t javaPort    = 25565;
    int      maxPlayers  = 8;
    bool     onlineMode  = false;
    const wchar_t* motd       = nullptr;
    const wchar_t* worldName  = nullptr;
    const wchar_t* gamemode   = nullptr;
    const wchar_t* difficulty = nullptr;
    int64_t  seed      = 0;
    bool     findSeed  = true;
};

// Thin C-style API so main.cpp doesn't need client headers
namespace DedicatedServer {
    bool init(const ServerInitParams& params);
    void tick();
    void shutdown();
    bool isRunning();
    int getPlayerCount();
}
