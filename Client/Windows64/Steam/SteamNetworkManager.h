#pragma once

#include "steam_api.h"
#include "../../Common/Network/PlatformNetworkManagerInterface.h"
#include "../../Common/Network/SessionInfo.h"
#include "../../../World/Math/C4JThread.h"
#include <vector>

// Steam-specific session info extension: stores the Steam lobby ID
// alongside the standard FriendSessionInfo data.
struct SteamSessionInfo
{
    CSteamID lobbyId;
    FriendSessionInfo friendInfo;
};

class CSteamNetworkManager : public CPlatformNetworkManager
{
    friend class CGameNetworkManager;
public:
    CSteamNetworkManager();
    ~CSteamNetworkManager();

    // CPlatformNetworkManager interface
    virtual bool Initialise(CGameNetworkManager *pGameNetworkManager, int flagIndexSize) override;
    virtual void Terminate() override;
    virtual int GetJoiningReadyPercentage() override;
    virtual int CorrectErrorIDS(int IDS) override;

    virtual void DoWork() override;
    virtual int GetPlayerCount() override;
    virtual int GetOnlinePlayerCount() override;
    virtual int GetLocalPlayerMask(int playerIndex) override;
    virtual bool AddLocalPlayerByUserIndex(int userIndex) override;
    virtual bool RemoveLocalPlayerByUserIndex(int userIndex) override;
    virtual INetworkPlayer *GetLocalPlayerByUserIndex(int userIndex) override;
    virtual INetworkPlayer *GetPlayerByIndex(int playerIndex) override;
    virtual INetworkPlayer *GetPlayerByXuid(PlayerUID xuid) override;
    virtual INetworkPlayer *GetPlayerBySmallId(unsigned char smallId) override;
    virtual bool ShouldMessageForFullSession() override;

    virtual INetworkPlayer *GetHostPlayer() override;
    virtual bool IsHost() override;
    virtual bool JoinGameFromInviteInfo(int userIndex, int userMask, const INVITE_INFO *pInviteInfo) override;
    virtual bool LeaveGame(bool bMigrateHost) override;

    virtual bool IsInSession() override;
    virtual bool IsInGameplay() override;
    virtual bool IsReadyToPlayOrIdle() override;
    virtual bool IsInStatsEnabledSession() override;
    virtual bool SessionHasSpace(unsigned int spaceRequired = 1) override;
    virtual void SendInviteGUI(int quadrant) override;
    virtual bool IsAddingPlayer() override;

    virtual void HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                          unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS,
                          unsigned char privateSlots = 0) override;
    virtual int JoinGame(FriendSessionInfo *searchResult, int localUsersMask, int primaryUserIndex) override;
    virtual bool SetLocalGame(bool isLocal) override;
    virtual bool IsLocalGame() override { return m_bIsOfflineGame; }
    virtual void SetPrivateGame(bool isPrivate) override;
    virtual bool IsPrivateGame() override { return m_bIsPrivateGame; }
    virtual bool IsLeavingGame() override { return m_bLeavingGame; }
    virtual void ResetLeavingGame() override { m_bLeavingGame = false; }

    virtual void RegisterPlayerChangedCallback(int iPad, void (*callback)(void *callbackParam, INetworkPlayer *pPlayer, bool leaving), void *callbackParam) override;
    virtual void UnRegisterPlayerChangedCallback(int iPad, void (*callback)(void *callbackParam, INetworkPlayer *pPlayer, bool leaving), void *callbackParam) override;

    virtual void HandleSignInChange() override;

    virtual bool _RunNetworkGame() override;

private:
    virtual bool _LeaveGame(bool bMigrateHost, bool bLeaveRoom) override;
    virtual void _HostGame(int usersMask, unsigned char publicSlots = MINECRAFT_NET_MAX_PLAYERS, unsigned char privateSlots = 0) override;
    virtual bool _StartGame() override;

public:
    virtual void UpdateAndSetGameSessionData(INetworkPlayer *pNetworkPlayerLeaving = NULL) override;

private:
    virtual bool RemoveLocalPlayer(INetworkPlayer *pNetworkPlayer) override;

public:
    virtual void SystemFlagSet(INetworkPlayer *pNetworkPlayer, int index) override;
    virtual bool SystemFlagGet(INetworkPlayer *pNetworkPlayer, int index) override;

    virtual std::wstring GatherStats() override;
    virtual std::wstring GatherRTTStats() override;

private:
    virtual void SetSessionTexturePackParentId(int id) override;
    virtual void SetSessionSubTexturePackId(int id) override;
    virtual void Notify(int ID, ULONG_PTR Param) override;

public:
    virtual std::vector<FriendSessionInfo *> *GetSessionList(int iPad, int localPlayers, bool partyOnly) override;
    virtual bool GetGameSessionInfo(int iPad, SessionID sessionId, FriendSessionInfo *foundSession) override;
    virtual void SetSessionsUpdatedCallback(void (*SessionsUpdatedCallback)(LPVOID pParam), LPVOID pSearchParam) override;
    virtual void GetFullFriendSessionInfo(FriendSessionInfo *foundSession, void (*FriendSessionUpdatedFn)(bool success, void *pParam), void *pParam) override;
    virtual void ForceFriendsSessionRefresh() override;

    // Steam-specific
    void NotifyPlayerJoined(IQNetPlayer *pQNetPlayer);
    void NotifyPlayerLeaving(IQNetPlayer *pQNetPlayer);

    // Lobby callbacks (Task 4)
    void OnLobbyCreated(LobbyCreated_t *pResult, bool bIOFailure);
    void OnLobbyMatchList(LobbyMatchList_t *pResult, bool bIOFailure);
    void OnLobbyEnter(LobbyEnter_t *pResult, bool bIOFailure);

    // Connection status callback
    STEAM_CALLBACK(CSteamNetworkManager, OnConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

private:
    IQNet *m_pIQNet;
    CGameNetworkManager *m_pGameNetworkManager;

    bool m_bLeavingGame;
    bool m_bLeaveGameOnTick;
    bool m_migrateHostOnLeave;
    bool m_bHostChanged;
    bool m_bIsOfflineGame;
    bool m_bIsPrivateGame;
    int  m_flagIndexSize;

    GameSessionData m_hostGameSessionData;

    // Steam Networking Sockets
    ISteamNetworkingSockets *m_pSteamNetSockets;
    HSteamListenSocket m_listenSocket;
    HSteamNetPollGroup m_pollGroup;
    HSteamNetConnection m_connections[MINECRAFT_NET_MAX_PLAYERS];

    // Steam Lobby
    CSteamID m_currentLobby;
    bool m_bLobbyCreated;

    // Call results for async operations
    CCallResult<CSteamNetworkManager, LobbyCreated_t> m_callResultLobbyCreated;
    CCallResult<CSteamNetworkManager, LobbyMatchList_t> m_callResultLobbyList;
    CCallResult<CSteamNetworkManager, LobbyEnter_t> m_callResultLobbyEnter;

    // Player tracking
    std::vector<IQNetPlayer *> m_machineQNetPrimaryPlayers;
    std::vector<INetworkPlayer *> currentNetworkPlayers;

    INetworkPlayer *addNetworkPlayer(IQNetPlayer *pQNetPlayer);
    void removeNetworkPlayer(IQNetPlayer *pQNetPlayer);
    static INetworkPlayer *getNetworkPlayer(IQNetPlayer *pQNetPlayer);

    // Player changed callbacks
    void (*playerChangedCallback[XUSER_MAX_COUNT])(void *callbackParam, INetworkPlayer *pPlayer, bool leaving);
    void *playerChangedCallbackParam[XUSER_MAX_COUNT];

    // System flags
    class PlayerFlags
    {
    public:
        INetworkPlayer *m_pNetworkPlayer;
        unsigned char *flags;
        unsigned int count;
        PlayerFlags(INetworkPlayer *pNetworkPlayer, unsigned int count);
        ~PlayerFlags();
    };
    std::vector<PlayerFlags *> m_playerFlags;
    void SystemFlagAddPlayer(INetworkPlayer *pNetworkPlayer);
    void SystemFlagRemovePlayer(INetworkPlayer *pNetworkPlayer);
    void SystemFlagReset();

    // Session search
    std::vector<FriendSessionInfo *> friendsSessions[XUSER_MAX_COUNT];
    int m_searchResultsCount[XUSER_MAX_COUNT];
    LPVOID m_pSearchParam;
    void (*m_SessionsUpdatedCallback)(LPVOID pParam);
    std::vector<SteamSessionInfo> m_steamSessions;

    void TickSearch();
    void RefreshSessionList();
};
