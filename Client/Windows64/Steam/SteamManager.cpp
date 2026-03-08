#include "SteamManager.h"
#include <stdio.h>

// ============================================================================
// SteamManager - Steam initialization, callbacks, and ID mapping.
// Uses Spacewar App ID 480 for dev/testing like FiveM does.
// ============================================================================

// Static members
bool SteamManager::s_initialized = false;
CSteamID SteamManager::s_localSteamId;
char SteamManager::s_localPersonaName[128] = { 0 };

// Bit 62 sentinel for Steam-derived XUIDs
static const ULONGLONG STEAM_XUID_BIT = 0x4000000000000000ULL;

// ---- Lifecycle ----

bool SteamManager::Initialize()
{
    if (s_initialized)
        return true;

    // Check if we need to restart through Steam
    if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID))
    {
        printf("[Steam] Restarting through Steam client...\n");
        return false;
    }

    // Initialize the Steam API
    if (!SteamAPI_Init())
    {
        printf("[Steam] SteamAPI_Init failed - Steam not running or not installed\n");
        printf("[Steam] Online features will be disabled, LAN play still available\n");
        return false;
    }

    // Get local user info
    ISteamUser* pUser = SteamUser();
    if (pUser && pUser->BLoggedOn())
    {
        s_localSteamId = pUser->GetSteamID();
        printf("[Steam] Logged in as Steam ID: %llu\n", s_localSteamId.ConvertToUint64());
    }

    ISteamFriends* pFriends = SteamFriends();
    if (pFriends)
    {
        const char* name = pFriends->GetPersonaName();
        if (name)
        {
            strncpy_s(s_localPersonaName, name, _TRUNCATE);
            printf("[Steam] Persona name: %s\n", s_localPersonaName);
        }
    }

    // Set rich presence
    if (pFriends)
    {
        pFriends->SetRichPresence("status", "In Menus");
    }

    s_initialized = true;
    printf("[Steam] Initialized successfully (App ID %d / Spacewar)\n", STEAM_APP_ID);
    return true;
}

void SteamManager::Shutdown()
{
    if (!s_initialized)
        return;

    ISteamFriends* pFriends = SteamFriends();
    if (pFriends)
        pFriends->ClearRichPresence();

    SteamAPI_Shutdown();
    s_initialized = false;
    s_localSteamId = CSteamID(0);
    s_localPersonaName[0] = '\0';

    printf("[Steam] Shutdown complete\n");
}

void SteamManager::Tick()
{
    if (!s_initialized)
        return;

    SteamAPI_RunCallbacks();
}

// ---- State ----

bool SteamManager::IsInitialized()
{
    return s_initialized;
}

CSteamID SteamManager::GetLocalSteamID()
{
    return s_localSteamId;
}

const char* SteamManager::GetLocalPersonaName()
{
    return s_localPersonaName;
}

// ---- ID Mapping ----
// Steam IDs are 64-bit. We use bit 62 as a sentinel to distinguish from:
//   - Real Xbox XUIDs (no high bits set)
//   - Discord XUIDs (bit 63 set, per DiscordXUIDMapper)

PlayerUID SteamManager::SteamIdToXuid(CSteamID steamId)
{
    return (PlayerUID)(STEAM_XUID_BIT | steamId.ConvertToUint64());
}

CSteamID SteamManager::XuidToSteamId(PlayerUID xuid)
{
    return CSteamID(xuid & ~STEAM_XUID_BIT);
}

bool SteamManager::IsSteamXuid(PlayerUID xuid)
{
    // Bit 62 set, bit 63 not set (bit 63 = Discord)
    return (xuid & STEAM_XUID_BIT) != 0 && (xuid & 0x8000000000000000ULL) == 0;
}

// ---- Friends ----

int SteamManager::GetFriendCount()
{
    if (!s_initialized) return 0;
    ISteamFriends* pFriends = SteamFriends();
    return pFriends ? pFriends->GetFriendCount(k_EFriendFlagImmediate) : 0;
}

CSteamID SteamManager::GetFriendByIndex(int index)
{
    if (!s_initialized) return CSteamID(0);
    ISteamFriends* pFriends = SteamFriends();
    return pFriends ? pFriends->GetFriendByIndex(index, k_EFriendFlagImmediate) : CSteamID(0);
}

const char* SteamManager::GetFriendPersonaName(CSteamID steamId)
{
    if (!s_initialized) return "";
    ISteamFriends* pFriends = SteamFriends();
    return pFriends ? pFriends->GetFriendPersonaName(steamId) : "";
}

bool SteamManager::IsFriendInGame(CSteamID steamId, FriendGameInfo_t* gameInfo)
{
    if (!s_initialized) return false;
    ISteamFriends* pFriends = SteamFriends();
    return pFriends ? pFriends->GetFriendGamePlayed(steamId, gameInfo) : false;
}

bool SteamManager::IsFriendOnline(CSteamID steamId)
{
    if (!s_initialized) return false;
    ISteamFriends* pFriends = SteamFriends();
    if (!pFriends) return false;
    EPersonaState state = pFriends->GetFriendPersonaState(steamId);
    return state != k_EPersonaStateOffline;
}

// ---- Utility ----

void SteamManager::PersonaNameToGamertag(const char* personaName, wchar_t* gamertagOut, int gamertagMaxLen)
{
    if (!personaName || !gamertagOut || gamertagMaxLen <= 0) return;

    int written = MultiByteToWideChar(CP_UTF8, 0, personaName, -1,
                                       gamertagOut, gamertagMaxLen);
    if (written == 0)
    {
        // Fallback: ASCII copy
        int i = 0;
        for (; personaName[i] && i < gamertagMaxLen - 1; ++i)
            gamertagOut[i] = (wchar_t)personaName[i];
        gamertagOut[i] = L'\0';
    }
}
