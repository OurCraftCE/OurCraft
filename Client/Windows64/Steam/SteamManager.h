#pragma once

#include "steam_api.h"
#include "../../../World/x64headers/extraX64.h"

// SteamManager - Singleton for Steam initialization, shutdown, and per-frame callbacks.
// Uses App ID 480 (Spacewar) for development/testing.
//
// Steam ID to XUID mapping: uses bit 62 as sentinel to distinguish from
// Discord XUIDs (bit 63) and real Xbox XUIDs (neither bit set).

#define STEAM_APP_ID 480

class SteamManager
{
public:
    // Lifecycle
    static bool Initialize();
    static void Shutdown();
    static void Tick(); // Call every frame - runs Steam callbacks

    // State
    static bool IsInitialized();
    static CSteamID GetLocalSteamID();
    static const char* GetLocalPersonaName();

    // Map Steam ID <-> PlayerUID (XUID)
    // Sets bit 62 to mark Steam-derived XUIDs.
    // Bit 63 is used by Discord, so this is a safe sentinel.
    static PlayerUID SteamIdToXuid(CSteamID steamId);
    static CSteamID XuidToSteamId(PlayerUID xuid);
    static bool IsSteamXuid(PlayerUID xuid);

    // Friends
    static int GetFriendCount();
    static CSteamID GetFriendByIndex(int index);
    static const char* GetFriendPersonaName(CSteamID steamId);
    static bool IsFriendInGame(CSteamID steamId, FriendGameInfo_t* gameInfo);
    static bool IsFriendOnline(CSteamID steamId);

    // Populate a gamertag buffer from a Steam persona name
    static void PersonaNameToGamertag(const char* personaName, wchar_t* gamertagOut, int gamertagMaxLen = 32);

private:
    SteamManager() = delete;

    static bool s_initialized;
    static CSteamID s_localSteamId;
    static char s_localPersonaName[128];
};
