// ============================================================================
// STUB/PLACEHOLDER - Steamworks API Header
// Replace with the real steam_api.h from the Steamworks SDK when available.
// https://partner.steamgames.com/doc/sdk
//
// Uses Spacewar App ID 480 for development/testing (like FiveM).
// Anyone with Steam installed can test multiplayer without a real App ID.
// ============================================================================
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

// ---- Basic Types ----

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int32 HSteamPipe;
typedef int32 HSteamUser;

// ---- CSteamID ----

class CSteamID
{
public:
    CSteamID() : m_steamid(0) {}
    CSteamID(uint64 id) : m_steamid(id) {}
    uint64 ConvertToUint64() const { return m_steamid; }
    bool IsValid() const { return m_steamid != 0; }
    bool operator==(const CSteamID& other) const { return m_steamid == other.m_steamid; }
    bool operator!=(const CSteamID& other) const { return m_steamid != other.m_steamid; }
    bool operator<(const CSteamID& other) const { return m_steamid < other.m_steamid; }
private:
    uint64 m_steamid;
};

// ---- Steam Networking Types ----

typedef uint32 HSteamListenSocket;
typedef uint32 HSteamNetConnection;
typedef uint32 HSteamNetPollGroup;

const HSteamListenSocket k_HSteamListenSocket_Invalid = 0;
const HSteamNetConnection k_HSteamNetConnection_Invalid = 0;
const HSteamNetPollGroup k_HSteamNetPollGroup_Invalid = 0;

// ---- Lobby Types ----

typedef uint64 SteamAPICall_t;
const SteamAPICall_t k_uAPICallInvalid = 0;

// ---- Enums ----

enum EResult
{
    k_EResultOK = 1,
    k_EResultFail = 2,
    k_EResultNoConnection = 3,
    k_EResultInvalidParam = 8,
    k_EResultTimeout = 16,
    k_EResultBusy = 10,
    k_EResultLimitExceeded = 25,
};

enum ESteamNetworkingConnectionState
{
    k_ESteamNetworkingConnectionState_None = 0,
    k_ESteamNetworkingConnectionState_Connecting = 1,
    k_ESteamNetworkingConnectionState_FindingRoute = 2,
    k_ESteamNetworkingConnectionState_Connected = 3,
    k_ESteamNetworkingConnectionState_ClosedByPeer = 4,
    k_ESteamNetworkingConnectionState_ProblemDetectedLocally = 5,
};

enum ESteamNetworkingSend
{
    k_nSteamNetworkingSend_Unreliable = 0,
    k_nSteamNetworkingSend_NoNagle = 1,
    k_nSteamNetworkingSend_UnreliableNoNagle = k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoNagle,
    k_nSteamNetworkingSend_NoDelay = 4,
    k_nSteamNetworkingSend_UnreliableNoDelay = k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoDelay | k_nSteamNetworkingSend_NoNagle,
    k_nSteamNetworkingSend_Reliable = 8,
    k_nSteamNetworkingSend_ReliableNoNagle = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle,
};

enum ESteamNetworkingConfigValue
{
    k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged = 201,
};

enum ELobbyType
{
    k_ELobbyTypePrivate = 0,
    k_ELobbyTypeFriendsOnly = 1,
    k_ELobbyTypePublic = 2,
    k_ELobbyTypeInvisible = 3,
};

enum ELobbyDistanceFilter
{
    k_ELobbyDistanceFilterClose = 0,
    k_ELobbyDistanceFilterDefault = 1,
    k_ELobbyDistanceFilterFar = 2,
    k_ELobbyDistanceFilterWorldwide = 3,
};

enum ELobbyComparison
{
    k_ELobbyComparisonEqualToOrLessThan = -2,
    k_ELobbyComparisonLessThan = -1,
    k_ELobbyComparisonEqual = 0,
    k_ELobbyComparisonGreaterThan = 1,
    k_ELobbyComparisonEqualToOrGreaterThan = 2,
    k_ELobbyComparisonNotEqual = 3,
};

enum EPersonaState
{
    k_EPersonaStateOffline = 0,
    k_EPersonaStateOnline = 1,
    k_EPersonaStateBusy = 2,
    k_EPersonaStateAway = 3,
    k_EPersonaStateSnooze = 4,
    k_EPersonaStateLookingToTrade = 5,
    k_EPersonaStateLookingToPlay = 6,
};

enum EFriendFlags
{
    k_EFriendFlagImmediate = 0x04,
    k_EFriendFlagAll = 0xFFFF,
};

// ---- Networking Address ----

struct SteamNetworkingIPAddr
{
    void Clear() { memset(this, 0, sizeof(*this)); }
    bool IsIPv4() const { return true; } // stub
    void SetIPv4(uint32 nIP, uint16 nPort) { m_ipv4 = nIP; m_port = nPort; }
    uint32 GetIPv4() const { return m_ipv4; }

    bool ParseString(const char* pszAddr)
    {
        // Simple IPv4:port parser
        if (!pszAddr) return false;
        char ip[64];
        strncpy_s(ip, pszAddr, _TRUNCATE);
        char* colon = strrchr(ip, ':');
        if (colon) {
            *colon = '\0';
            m_port = (uint16)atoi(colon + 1);
        }
        // Convert dotted IP to uint32
        unsigned int a, b, c, d;
        if (sscanf_s(ip, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            m_ipv4 = (a << 24) | (b << 16) | (c << 8) | d;
            return true;
        }
        return false;
    }

    void ToString(char* buf, size_t cbBuf, bool bWithPort) const
    {
        if (bWithPort)
            sprintf_s(buf, cbBuf, "%u.%u.%u.%u:%u",
                (m_ipv4 >> 24) & 0xFF, (m_ipv4 >> 16) & 0xFF,
                (m_ipv4 >> 8) & 0xFF, m_ipv4 & 0xFF, m_port);
        else
            sprintf_s(buf, cbBuf, "%u.%u.%u.%u",
                (m_ipv4 >> 24) & 0xFF, (m_ipv4 >> 16) & 0xFF,
                (m_ipv4 >> 8) & 0xFF, m_ipv4 & 0xFF);
    }

    uint16 m_port;
    uint32 m_ipv4;
};

// ---- Networking Identity ----

struct SteamNetworkingIdentity
{
    void Clear() { memset(this, 0, sizeof(*this)); }
    void SetSteamID(CSteamID steamID) { m_steamID = steamID; }
    CSteamID GetSteamID() const { return m_steamID; }
    void SetIPAddr(const SteamNetworkingIPAddr& addr) { m_ipAddr = addr; }

    CSteamID m_steamID;
    SteamNetworkingIPAddr m_ipAddr;
};

// ---- Connection Info / Status ----

struct SteamNetConnectionInfo_t
{
    SteamNetworkingIdentity m_identityRemote;
    ESteamNetworkingConnectionState m_eState;
    int m_eEndReason;
    char m_szEndDebug[128];
    char m_szConnectionDescription[128];
};

struct SteamNetConnectionRealTimeStatus_t
{
    ESteamNetworkingConnectionState m_eState;
    int m_nPing;
    float m_flConnectionQualityLocal;
    float m_flConnectionQualityRemote;
    float m_flOutPacketsPerSec;
    float m_flOutBytesPerSec;
    float m_flInPacketsPerSec;
    float m_flInBytesPerSec;
    int m_cbPendingUnreliable;
    int m_cbPendingReliable;
    int m_cbSentUnackedReliable;
};

// ---- Config Value ----

struct SteamNetworkingConfigValue_t
{
    ESteamNetworkingConfigValue m_eValue;
    int64_t m_val;  // union stand-in

    void SetPtr(ESteamNetworkingConfigValue eVal, void* ptr)
    {
        m_eValue = eVal;
        m_val = (int64_t)ptr;
    }
};

// ---- Networking Message ----

struct SteamNetworkingMessage_t
{
    void* m_pData;
    uint32 m_cbSize;
    HSteamNetConnection m_conn;
    SteamNetworkingIdentity m_identityPeer;
    int64_t m_nConnUserData;
    int m_nChannel;

    void Release() {} // stub
};

// ---- Connection Status Changed Callback ----

struct SteamNetConnectionStatusChangedCallback_t
{
    HSteamNetConnection m_hConn;
    SteamNetConnectionInfo_t m_info;
    ESteamNetworkingConnectionState m_eOldState;
};

// ---- Friend Game Info ----

struct FriendGameInfo_t
{
    uint64 m_gameID;
    uint32 m_unGameIP;
    uint16 m_usGamePort;
    uint16 m_usQueryPort;
    CSteamID m_steamIDLobby;
};

// ---- Lobby Callbacks ----

struct LobbyCreated_t
{
    enum { k_iCallback = 513 };
    EResult m_eResult;
    uint64 m_ulSteamIDLobby;
};

struct LobbyMatchList_t
{
    enum { k_iCallback = 510 };
    uint32 m_nLobbiesMatching;
};

struct LobbyEnter_t
{
    enum { k_iCallback = 504 };
    uint64 m_ulSteamIDLobby;
    bool m_bLocked;
    uint32 m_EChatRoomEnterResponse;
};

struct LobbyChatUpdate_t
{
    enum { k_iCallback = 506 };
    uint64 m_ulSteamIDLobby;
    uint64 m_ulSteamIDUserChanged;
    uint64 m_ulSteamIDMakingChange;
    uint32 m_rgfChatMemberStateChange;
};

struct LobbyDataUpdate_t
{
    enum { k_iCallback = 505 };
    uint64 m_ulSteamIDLobby;
    uint64 m_ulSteamIDMember;
    uint8 m_bSuccess;
};

struct GameLobbyJoinRequested_t
{
    enum { k_iCallback = 333 };
    CSteamID m_steamIDLobby;
    CSteamID m_steamIDFriend;
};

// ---- CCallResult / STEAM_CALLBACK helpers ----

// CCallResult: used to track async Steam API calls
template<class T, class P>
class CCallResult
{
public:
    typedef void (T::*func_t)(P*, bool);

    CCallResult() : m_hAPICall(k_uAPICallInvalid), m_pObj(nullptr), m_Func(nullptr) {}

    void Set(SteamAPICall_t hAPICall, T* p, func_t func)
    {
        m_hAPICall = hAPICall;
        m_pObj = p;
        m_Func = func;
    }

    bool IsActive() const { return m_hAPICall != k_uAPICallInvalid; }
    void Cancel() { m_hAPICall = k_uAPICallInvalid; }

    SteamAPICall_t m_hAPICall;
    T* m_pObj;
    func_t m_Func;
};

// STEAM_CALLBACK macro: register a member function to receive a Steam callback
#define STEAM_CALLBACK(thisclass, func, param) \
    void func(param* pParam)

// ---- ISteamNetworkingSockets Interface ----

class ISteamNetworkingSockets
{
public:
    virtual HSteamListenSocket CreateListenSocketIP(const SteamNetworkingIPAddr& localAddress, int nOptions, const SteamNetworkingConfigValue_t* pOptions) = 0;
    virtual HSteamNetConnection ConnectByIPAddress(const SteamNetworkingIPAddr& address, int nOptions, const SteamNetworkingConfigValue_t* pOptions) = 0;
    virtual EResult AcceptConnection(HSteamNetConnection hConn) = 0;
    virtual bool CloseConnection(HSteamNetConnection hPeer, int nReason, const char* pszDebug, bool bEnableLinger) = 0;
    virtual bool CloseListenSocket(HSteamListenSocket hSocket) = 0;
    virtual bool SetConnectionUserData(HSteamNetConnection hPeer, int64_t nUserData) = 0;
    virtual int64_t GetConnectionUserData(HSteamNetConnection hPeer) = 0;
    virtual EResult SendMessageToConnection(HSteamNetConnection hConn, const void* pData, uint32 cbData, int nSendFlags, int64_t* pOutMessageNumber) = 0;
    virtual int ReceiveMessagesOnConnection(HSteamNetConnection hConn, SteamNetworkingMessage_t** ppOutMessages, int nMaxMessages) = 0;
    virtual bool GetConnectionInfo(HSteamNetConnection hConn, SteamNetConnectionInfo_t* pInfo) = 0;
    virtual bool GetConnectionRealTimeStatus(HSteamNetConnection hConn, SteamNetConnectionRealTimeStatus_t* pStatus, int nLanes, void* pLanes) = 0;
    virtual HSteamNetPollGroup CreatePollGroup() = 0;
    virtual bool DestroyPollGroup(HSteamNetPollGroup hPollGroup) = 0;
    virtual bool SetConnectionPollGroup(HSteamNetConnection hConn, HSteamNetPollGroup hPollGroup) = 0;
    virtual int ReceiveMessagesOnPollGroup(HSteamNetPollGroup hPollGroup, SteamNetworkingMessage_t** ppOutMessages, int nMaxMessages) = 0;
};

// ---- ISteamMatchmaking Interface ----

class ISteamMatchmaking
{
public:
    virtual SteamAPICall_t CreateLobby(ELobbyType eLobbyType, int cMaxMembers) = 0;
    virtual SteamAPICall_t JoinLobby(CSteamID steamIDLobby) = 0;
    virtual void LeaveLobby(CSteamID steamIDLobby) = 0;
    virtual bool SetLobbyData(CSteamID steamIDLobby, const char* pchKey, const char* pchValue) = 0;
    virtual const char* GetLobbyData(CSteamID steamIDLobby, const char* pchKey) = 0;
    virtual int GetLobbyDataCount(CSteamID steamIDLobby) = 0;
    virtual CSteamID GetLobbyOwner(CSteamID steamIDLobby) = 0;
    virtual int GetNumLobbyMembers(CSteamID steamIDLobby) = 0;
    virtual CSteamID GetLobbyMemberByIndex(CSteamID steamIDLobby, int iMember) = 0;
    virtual bool SetLobbyMemberLimit(CSteamID steamIDLobby, int cMaxMembers) = 0;
    virtual int GetLobbyMemberLimit(CSteamID steamIDLobby) = 0;
    virtual void SetLobbyType(CSteamID steamIDLobby, ELobbyType eLobbyType) = 0;
    virtual bool SetLobbyJoinable(CSteamID steamIDLobby, bool bLobbyJoinable) = 0;
    virtual SteamAPICall_t RequestLobbyList() = 0;
    virtual void AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter eLobbyDistanceFilter) = 0;
    virtual void AddRequestLobbyListStringFilter(const char* pchKeyToMatch, const char* pchValueToMatch, ELobbyComparison eComparisonType) = 0;
    virtual void AddRequestLobbyListNumericalFilter(const char* pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType) = 0;
    virtual void AddRequestLobbyListResultCountFilter(int cMaxResults) = 0;
    virtual CSteamID GetLobbyByIndex(int iLobby) = 0;
    virtual bool SendLobbyChatMsg(CSteamID steamIDLobby, const void* pvMsgBody, int cubMsgBody) = 0;
    virtual bool InviteUserToLobby(CSteamID steamIDLobby, CSteamID steamIDInvitee) = 0;
};

// ---- ISteamFriends Interface ----

class ISteamFriends
{
public:
    virtual const char* GetPersonaName() = 0;
    virtual int GetFriendCount(int iFriendFlags) = 0;
    virtual CSteamID GetFriendByIndex(int iFriend, int iFriendFlags) = 0;
    virtual const char* GetFriendPersonaName(CSteamID steamIDFriend) = 0;
    virtual EPersonaState GetFriendPersonaState(CSteamID steamIDFriend) = 0;
    virtual bool GetFriendGamePlayed(CSteamID steamIDFriend, FriendGameInfo_t* pFriendGameInfo) = 0;
    virtual void SetRichPresence(const char* pchKey, const char* pchValue) = 0;
    virtual void ClearRichPresence() = 0;
    virtual void ActivateGameOverlay(const char* pchDialog) = 0;
    virtual void ActivateGameOverlayInviteDialog(CSteamID steamIDLobby) = 0;
};

// ---- ISteamUser Interface ----

class ISteamUser
{
public:
    virtual CSteamID GetSteamID() = 0;
    virtual bool BLoggedOn() = 0;
};

// ---- ISteamUtils Interface ----

class ISteamUtils
{
public:
    virtual uint32 GetAppID() = 0;
    virtual const char* GetIPCountry() = 0;
    virtual bool IsOverlayEnabled() = 0;
};

// ---- Global API Functions ----

// These are declared here and defined as no-op stubs in steam_api_stub.cpp.
// Replace steam_api_stub.cpp with the real steam_api64.lib linkage when the SDK is available.

bool SteamAPI_Init();
void SteamAPI_Shutdown();
void SteamAPI_RunCallbacks();
bool SteamAPI_RestartAppIfNecessary(uint32 unOwnAppID);

ISteamUser* SteamUser();
ISteamFriends* SteamFriends();
ISteamMatchmaking* SteamMatchmaking();
ISteamNetworkingSockets* SteamNetworkingSockets();
ISteamUtils* SteamUtils();

// Networking Sockets utility
void SteamNetworkingIPAddr_ToString(const SteamNetworkingIPAddr* pAddr, char* buf, size_t cbBuf, bool bWithPort);

// end of steam_api.h
