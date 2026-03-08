// ============================================================================
// STUB - Steamworks API No-Op Implementation
// Replace this file by linking steam_api64.lib from the real Steamworks SDK.
// All functions return failure/empty values so the game runs without Steam.
// ============================================================================

#include "steam_api.h"
#include <stdio.h>

// ---- Stub ISteamUser ----

class CSteamUserStub : public ISteamUser
{
public:
    CSteamID GetSteamID() override { return CSteamID(0); }
    bool BLoggedOn() override { return false; }
};

// ---- Stub ISteamFriends ----

class CSteamFriendsStub : public ISteamFriends
{
public:
    const char* GetPersonaName() override { return "Player"; }
    int GetFriendCount(int) override { return 0; }
    CSteamID GetFriendByIndex(int, int) override { return CSteamID(0); }
    const char* GetFriendPersonaName(CSteamID) override { return ""; }
    EPersonaState GetFriendPersonaState(CSteamID) override { return k_EPersonaStateOffline; }
    bool GetFriendGamePlayed(CSteamID, FriendGameInfo_t*) override { return false; }
    void SetRichPresence(const char*, const char*) override {}
    void ClearRichPresence() override {}
    void ActivateGameOverlay(const char*) override {}
    void ActivateGameOverlayInviteDialog(CSteamID) override {}
};

// ---- Stub ISteamMatchmaking ----

class CSteamMatchmakingStub : public ISteamMatchmaking
{
public:
    SteamAPICall_t CreateLobby(ELobbyType, int) override { return k_uAPICallInvalid; }
    SteamAPICall_t JoinLobby(CSteamID) override { return k_uAPICallInvalid; }
    void LeaveLobby(CSteamID) override {}
    bool SetLobbyData(CSteamID, const char*, const char*) override { return false; }
    const char* GetLobbyData(CSteamID, const char*) override { return ""; }
    int GetLobbyDataCount(CSteamID) override { return 0; }
    CSteamID GetLobbyOwner(CSteamID) override { return CSteamID(0); }
    int GetNumLobbyMembers(CSteamID) override { return 0; }
    CSteamID GetLobbyMemberByIndex(CSteamID, int) override { return CSteamID(0); }
    bool SetLobbyMemberLimit(CSteamID, int) override { return false; }
    int GetLobbyMemberLimit(CSteamID) override { return 0; }
    void SetLobbyType(CSteamID, ELobbyType) override {}
    bool SetLobbyJoinable(CSteamID, bool) override { return false; }
    SteamAPICall_t RequestLobbyList() override { return k_uAPICallInvalid; }
    void AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter) override {}
    void AddRequestLobbyListStringFilter(const char*, const char*, ELobbyComparison) override {}
    void AddRequestLobbyListNumericalFilter(const char*, int, ELobbyComparison) override {}
    void AddRequestLobbyListResultCountFilter(int) override {}
    CSteamID GetLobbyByIndex(int) override { return CSteamID(0); }
    bool SendLobbyChatMsg(CSteamID, const void*, int) override { return false; }
    bool InviteUserToLobby(CSteamID, CSteamID) override { return false; }
};

// ---- Stub ISteamNetworkingSockets ----

class CSteamNetworkingSocketsStub : public ISteamNetworkingSockets
{
public:
    HSteamListenSocket CreateListenSocketIP(const SteamNetworkingIPAddr&, int, const SteamNetworkingConfigValue_t*) override { return k_HSteamListenSocket_Invalid; }
    HSteamNetConnection ConnectByIPAddress(const SteamNetworkingIPAddr&, int, const SteamNetworkingConfigValue_t*) override { return k_HSteamNetConnection_Invalid; }
    EResult AcceptConnection(HSteamNetConnection) override { return k_EResultFail; }
    bool CloseConnection(HSteamNetConnection, int, const char*, bool) override { return false; }
    bool CloseListenSocket(HSteamListenSocket) override { return false; }
    bool SetConnectionUserData(HSteamNetConnection, int64_t) override { return false; }
    int64_t GetConnectionUserData(HSteamNetConnection) override { return -1; }
    EResult SendMessageToConnection(HSteamNetConnection, const void*, uint32, int, int64_t*) override { return k_EResultFail; }
    int ReceiveMessagesOnConnection(HSteamNetConnection, SteamNetworkingMessage_t**, int) override { return 0; }
    bool GetConnectionInfo(HSteamNetConnection, SteamNetConnectionInfo_t*) override { return false; }
    bool GetConnectionRealTimeStatus(HSteamNetConnection, SteamNetConnectionRealTimeStatus_t*, int, void*) override { return false; }
    HSteamNetPollGroup CreatePollGroup() override { return k_HSteamNetPollGroup_Invalid; }
    bool DestroyPollGroup(HSteamNetPollGroup) override { return false; }
    bool SetConnectionPollGroup(HSteamNetConnection, HSteamNetPollGroup) override { return false; }
    int ReceiveMessagesOnPollGroup(HSteamNetPollGroup, SteamNetworkingMessage_t**, int) override { return 0; }
};

// ---- Stub ISteamUtils ----

class CSteamUtilsStub : public ISteamUtils
{
public:
    uint32 GetAppID() override { return 480; } // Spacewar
    const char* GetIPCountry() override { return "US"; }
    bool IsOverlayEnabled() override { return false; }
};

// ---- Singleton Instances ----

static CSteamUserStub s_steamUserStub;
static CSteamFriendsStub s_steamFriendsStub;
static CSteamMatchmakingStub s_steamMatchmakingStub;
static CSteamNetworkingSocketsStub s_steamNetSocketsStub;
static CSteamUtilsStub s_steamUtilsStub;

static bool s_steamInitialized = false;

// ---- Global API ----

bool SteamAPI_Init()
{
    printf("[Steam STUB] SteamAPI_Init called - using stub implementation\n");
    s_steamInitialized = true;
    return true; // Return true so the game thinks Steam is available
}

void SteamAPI_Shutdown()
{
    printf("[Steam STUB] SteamAPI_Shutdown called\n");
    s_steamInitialized = false;
}

void SteamAPI_RunCallbacks()
{
    // No-op in stub
}

bool SteamAPI_RestartAppIfNecessary(uint32 unOwnAppID)
{
    (void)unOwnAppID;
    return false; // Don't restart
}

ISteamUser* SteamUser()
{
    return &s_steamUserStub;
}

ISteamFriends* SteamFriends()
{
    return &s_steamFriendsStub;
}

ISteamMatchmaking* SteamMatchmaking()
{
    return &s_steamMatchmakingStub;
}

ISteamNetworkingSockets* SteamNetworkingSockets()
{
    return &s_steamNetSocketsStub;
}

ISteamUtils* SteamUtils()
{
    return &s_steamUtilsStub;
}

void SteamNetworkingIPAddr_ToString(const SteamNetworkingIPAddr* pAddr, char* buf, size_t cbBuf, bool bWithPort)
{
    if (pAddr)
        pAddr->ToString(buf, cbBuf, bWithPort);
    else if (buf && cbBuf > 0)
        buf[0] = '\0';
}
