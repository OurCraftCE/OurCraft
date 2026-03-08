#include "stdafx.h"
#include "../../../World/IO/Socket.h"
#include "../../../World/Math/StringHelpers.h"
#include "SteamNetworkManager.h"
#include "SteamManager.h"
#include "../../Common/Network/NetworkPlayerXbox.h"
#include "../../Windows64/Network/WinsockNetLayer.h"
#include "../../OurCraft.h"
#include "../../Game/User.h"

// ============================================================================
// CSteamNetworkManager - Steam Networking Sockets + Lobby based network manager
// Implements CPlatformNetworkManager for Steamworks multiplayer.
// Falls back to WinsockNetLayer for actual data transfer (Steam relay is used
// for lobby discovery and NAT traversal; game data goes over TCP like LAN).
// ============================================================================

CSteamNetworkManager::CSteamNetworkManager()
    : m_pIQNet(NULL)
    , m_pGameNetworkManager(NULL)
    , m_bLeavingGame(false)
    , m_bLeaveGameOnTick(false)
    , m_migrateHostOnLeave(false)
    , m_bHostChanged(false)
    , m_bIsOfflineGame(false)
    , m_bIsPrivateGame(false)
    , m_flagIndexSize(0)
    , m_pSteamNetSockets(NULL)
    , m_listenSocket(k_HSteamListenSocket_Invalid)
    , m_pollGroup(k_HSteamNetPollGroup_Invalid)
    , m_bLobbyCreated(false)
    , m_pSearchParam(NULL)
    , m_SessionsUpdatedCallback(NULL)
{
    memset(m_connections, 0, sizeof(m_connections));
}

CSteamNetworkManager::~CSteamNetworkManager()
{
}

// ---- Initialization / Termination ----

bool CSteamNetworkManager::Initialise(CGameNetworkManager *pGameNetworkManager, int flagIndexSize)
{
    m_pGameNetworkManager = pGameNetworkManager;
    m_flagIndexSize = flagIndexSize;
    m_pIQNet = new IQNet();

    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        playerChangedCallback[i] = NULL;
        playerChangedCallbackParam[i] = NULL;
    }

    m_bLeavingGame = false;
    m_bLeaveGameOnTick = false;
    m_bHostChanged = false;
    m_bIsOfflineGame = false;
    m_pSearchParam = NULL;
    m_SessionsUpdatedCallback = NULL;

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        m_searchResultsCount[i] = 0;
    }

    // Get Steam Networking Sockets interface
    if (SteamManager::IsInitialized())
    {
        m_pSteamNetSockets = SteamNetworkingSockets();
        app.DebugPrintf("[Steam] Network manager initialized with Steam Networking Sockets\n");
    }
    else
    {
        m_pSteamNetSockets = NULL;
        app.DebugPrintf("[Steam] Network manager initialized without Steam (offline mode)\n");
    }

    return true;
}

void CSteamNetworkManager::Terminate()
{
    if (m_listenSocket != k_HSteamListenSocket_Invalid && m_pSteamNetSockets)
    {
        m_pSteamNetSockets->CloseListenSocket(m_listenSocket);
        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    if (m_pollGroup != k_HSteamNetPollGroup_Invalid && m_pSteamNetSockets)
    {
        m_pSteamNetSockets->DestroyPollGroup(m_pollGroup);
        m_pollGroup = k_HSteamNetPollGroup_Invalid;
    }

    // Leave any active lobby
    if (m_currentLobby.IsValid())
    {
        SteamMatchmaking()->LeaveLobby(m_currentLobby);
        m_currentLobby = CSteamID(0);
    }
}

int CSteamNetworkManager::GetJoiningReadyPercentage()
{
    return 100;
}

int CSteamNetworkManager::CorrectErrorIDS(int IDS)
{
    return IDS;
}

// ---- DoWork (tick) ----

void CSteamNetworkManager::DoWork()
{
    extern QNET_STATE _iQNetStubState;

    // Transition from STARTING to GAME_PLAY when game is ready
    if (_iQNetStubState == QNET_STATE_SESSION_STARTING && app.GetGameStarted())
    {
        _iQNetStubState = QNET_STATE_GAME_PLAY;
        if (m_pIQNet->IsHost())
            WinsockNetLayer::UpdateAdvertiseJoinable(true);
    }

    // When idle, tick session search (lobby browsing)
    if (_iQNetStubState == QNET_STATE_IDLE)
        TickSearch();

    // Handle disconnects (host side)
    if (_iQNetStubState == QNET_STATE_GAME_PLAY && m_pIQNet->IsHost())
    {
        BYTE disconnectedSmallId;
        while (WinsockNetLayer::PopDisconnectedSmallId(&disconnectedSmallId))
        {
            IQNetPlayer *qnetPlayer = m_pIQNet->GetPlayerBySmallId(disconnectedSmallId);
            if (qnetPlayer != NULL && qnetPlayer->m_smallId == disconnectedSmallId)
            {
                NotifyPlayerLeaving(qnetPlayer);
                qnetPlayer->m_smallId = 0;
                qnetPlayer->m_isRemote = false;
                qnetPlayer->m_isHostPlayer = false;
                qnetPlayer->m_gamertag[0] = 0;
                qnetPlayer->SetCustomDataValue(0);
                WinsockNetLayer::PushFreeSmallId(disconnectedSmallId);
                if (IQNet::s_playerCount > 1)
                    IQNet::s_playerCount--;
            }
        }

        // Check for sockets closing
        for (int i = 1; i < MINECRAFT_NET_MAX_PLAYERS; i++)
        {
            IQNetPlayer *qp = &IQNet::m_player[i];
            if (qp->GetCustomDataValue() == 0)
                continue;
            INetworkPlayer *np = (INetworkPlayer *)qp->GetCustomDataValue();
            Socket *sock = np->GetSocket();
            if (sock != NULL && sock->isClosing())
            {
                WinsockNetLayer::CloseConnectionBySmallId((BYTE)i);
            }
        }
    }

    // Handle disconnects (client side)
    if (_iQNetStubState == QNET_STATE_GAME_PLAY && !m_pIQNet->IsHost())
    {
        if (!WinsockNetLayer::IsConnected() && !g_NetworkManager.IsLeavingGame())
        {
            if (app.GetDisconnectReason() == DisconnectPacket::eDisconnect_None)
                app.SetDisconnectReason(DisconnectPacket::eDisconnect_Quitting);
            app.SetAction(ProfileManager.GetPrimaryPad(), eAppAction_ExitWorld, (void *)TRUE);
        }
    }
}

// ---- Player Management ----

int CSteamNetworkManager::GetPlayerCount()
{
    return m_pIQNet->GetPlayerCount();
}

int CSteamNetworkManager::GetOnlinePlayerCount()
{
    return 1;
}

int CSteamNetworkManager::GetLocalPlayerMask(int playerIndex)
{
    return 1 << playerIndex;
}

bool CSteamNetworkManager::AddLocalPlayerByUserIndex(int userIndex)
{
    NotifyPlayerJoined(m_pIQNet->GetLocalPlayerByUserIndex(userIndex));
    return (m_pIQNet->AddLocalPlayerByUserIndex(userIndex) == S_OK);
}

bool CSteamNetworkManager::RemoveLocalPlayerByUserIndex(int userIndex)
{
    return true;
}

INetworkPlayer *CSteamNetworkManager::GetLocalPlayerByUserIndex(int userIndex)
{
    return getNetworkPlayer(m_pIQNet->GetLocalPlayerByUserIndex(userIndex));
}

INetworkPlayer *CSteamNetworkManager::GetPlayerByIndex(int playerIndex)
{
    return getNetworkPlayer(m_pIQNet->GetPlayerByIndex(playerIndex));
}

INetworkPlayer *CSteamNetworkManager::GetPlayerByXuid(PlayerUID xuid)
{
    return getNetworkPlayer(m_pIQNet->GetPlayerByXuid(xuid));
}

INetworkPlayer *CSteamNetworkManager::GetPlayerBySmallId(unsigned char smallId)
{
    IQNetPlayer *qnetPlayer = m_pIQNet->GetPlayerBySmallId(smallId);
    if (qnetPlayer == NULL)
        return NULL;

    INetworkPlayer *networkPlayer = getNetworkPlayer(qnetPlayer);
    if (networkPlayer == NULL && smallId != 0 && !m_pIQNet->IsHost())
    {
        qnetPlayer->m_isRemote = true;
        qnetPlayer->m_isHostPlayer = false;
        NotifyPlayerJoined(qnetPlayer);
        networkPlayer = getNetworkPlayer(qnetPlayer);
    }
    return networkPlayer;
}

bool CSteamNetworkManager::ShouldMessageForFullSession()
{
    return false;
}

INetworkPlayer *CSteamNetworkManager::GetHostPlayer()
{
    return getNetworkPlayer(m_pIQNet->GetHostPlayer());
}

bool CSteamNetworkManager::IsHost()
{
    return m_pIQNet->IsHost() && !m_bHostChanged;
}

bool CSteamNetworkManager::JoinGameFromInviteInfo(int userIndex, int userMask, const INVITE_INFO *pInviteInfo)
{
    return (m_pIQNet->JoinGameFromInviteInfo(userIndex, userMask, pInviteInfo) == S_OK);
}

// ---- Session State ----

bool CSteamNetworkManager::IsInSession()
{
    return m_pIQNet->GetState() != QNET_STATE_IDLE;
}

bool CSteamNetworkManager::IsInGameplay()
{
    return m_pIQNet->GetState() == QNET_STATE_GAME_PLAY;
}

bool CSteamNetworkManager::IsReadyToPlayOrIdle()
{
    return true;
}

bool CSteamNetworkManager::IsInStatsEnabledSession()
{
    return true;
}

bool CSteamNetworkManager::SessionHasSpace(unsigned int spaceRequired)
{
    return true;
}

void CSteamNetworkManager::SendInviteGUI(int quadrant)
{
    // Open Steam overlay invite dialog if we have a lobby
    if (m_currentLobby.IsValid() && SteamManager::IsInitialized())
    {
        SteamFriends()->ActivateGameOverlayInviteDialog(m_currentLobby);
    }
}

bool CSteamNetworkManager::IsAddingPlayer()
{
    return false;
}

// ---- Hosting ----

void CSteamNetworkManager::HostGame(int localUsersMask, bool bOnlineGame, bool bIsPrivate,
                                     unsigned char publicSlots, unsigned char privateSlots)
{
    SetLocalGame(!bOnlineGame);
    SetPrivateGame(bIsPrivate);
    SystemFlagReset();

    localUsersMask |= GetLocalPlayerMask(g_NetworkManager.GetPrimaryPad());

    m_bLeavingGame = false;

    m_pIQNet->HostGame();

    IQNet::m_player[0].m_smallId = 0;
    IQNet::m_player[0].m_isRemote = false;
    IQNet::m_player[0].m_isHostPlayer = true;
    IQNet::s_playerCount = 1;

    _HostGame(localUsersMask, publicSlots, privateSlots);

    // Start Winsock-based hosting for actual game data
    int port = WIN64_NET_DEFAULT_PORT;
    if (!WinsockNetLayer::IsActive())
        WinsockNetLayer::HostGame(port);

    const wchar_t *hostName = IQNet::m_player[0].m_gamertag;
    unsigned int settings = app.GetGameHostOption(eGameHostOption_All);
    WinsockNetLayer::StartAdvertising(port, hostName, settings, 0, 0, MINECRAFT_NET_VERSION);

    // Create a Steam lobby for discoverability (Task 4)
    if (SteamManager::IsInitialized() && bOnlineGame)
    {
        ELobbyType lobbyType = bIsPrivate ? k_ELobbyTypeFriendsOnly : k_ELobbyTypePublic;
        SteamAPICall_t hCall = SteamMatchmaking()->CreateLobby(lobbyType, publicSlots);
        m_callResultLobbyCreated.Set(hCall, this, &CSteamNetworkManager::OnLobbyCreated);
        app.DebugPrintf("[Steam] Creating %s lobby with %d slots\n",
                        bIsPrivate ? "friends-only" : "public", publicSlots);
    }
}

void CSteamNetworkManager::_HostGame(int usersMask, unsigned char publicSlots, unsigned char privateSlots)
{
}

bool CSteamNetworkManager::_StartGame()
{
    return true;
}

// ---- Joining ----

int CSteamNetworkManager::JoinGame(FriendSessionInfo *searchResult, int localUsersMask, int primaryUserIndex)
{
    if (searchResult == NULL)
        return CGameNetworkManager::JOINGAME_FAIL_GENERAL;

    const char *hostIP = searchResult->data.hostIP;
    int hostPort = searchResult->data.hostPort;

    if (hostPort <= 0 || hostIP[0] == 0)
        return CGameNetworkManager::JOINGAME_FAIL_GENERAL;

    m_bLeavingGame = false;
    IQNet::s_isHosting = false;
    m_pIQNet->ClientJoinGame();

    IQNet::m_player[0].m_smallId = 0;
    IQNet::m_player[0].m_isRemote = true;
    IQNet::m_player[0].m_isHostPlayer = true;
    wcsncpy_s(IQNet::m_player[0].m_gamertag, 32, searchResult->data.hostName, _TRUNCATE);

    WinsockNetLayer::StopDiscovery();

    if (!WinsockNetLayer::JoinGame(hostIP, hostPort))
    {
        app.DebugPrintf("[Steam] Failed to connect to %s:%d\n", hostIP, hostPort);
        return CGameNetworkManager::JOINGAME_FAIL_GENERAL;
    }

    BYTE localSmallId = WinsockNetLayer::GetLocalSmallId();

    IQNet::m_player[localSmallId].m_smallId = localSmallId;
    IQNet::m_player[localSmallId].m_isRemote = false;
    IQNet::m_player[localSmallId].m_isHostPlayer = false;

    Minecraft *pMinecraft = Minecraft::GetInstance();
    wcscpy_s(IQNet::m_player[localSmallId].m_gamertag, 32, pMinecraft->user->name.c_str());
    IQNet::s_playerCount = localSmallId + 1;

    NotifyPlayerJoined(&IQNet::m_player[0]);
    NotifyPlayerJoined(&IQNet::m_player[localSmallId]);

    m_pGameNetworkManager->StateChange_AnyToStarting();

    app.DebugPrintf("[Steam] Joined game at %s:%d as player %d\n", hostIP, hostPort, localSmallId);

    return CGameNetworkManager::JOINGAME_SUCCESS;
}

// ---- Leaving ----

bool CSteamNetworkManager::LeaveGame(bool bMigrateHost)
{
    if (m_bLeavingGame) return true;

    m_bLeavingGame = true;

    WinsockNetLayer::StopAdvertising();

    // Leave Steam lobby
    if (m_currentLobby.IsValid())
    {
        SteamMatchmaking()->LeaveLobby(m_currentLobby);
        m_currentLobby = CSteamID(0);
        m_bLobbyCreated = false;
        app.DebugPrintf("[Steam] Left lobby\n");
    }

    if (m_pIQNet->IsHost() && g_NetworkManager.ServerStoppedValid())
    {
        m_pIQNet->EndGame();
        g_NetworkManager.ServerStoppedWait();
        g_NetworkManager.ServerStoppedDestroy();
    }
    else
    {
        m_pIQNet->EndGame();
    }

    for (auto it = currentNetworkPlayers.begin(); it != currentNetworkPlayers.end(); it++)
        delete *it;
    currentNetworkPlayers.clear();
    m_machineQNetPrimaryPlayers.clear();
    SystemFlagReset();

    WinsockNetLayer::Shutdown();
    WinsockNetLayer::Initialize();

    return true;
}

bool CSteamNetworkManager::_LeaveGame(bool bMigrateHost, bool bLeaveRoom)
{
    return true;
}

bool CSteamNetworkManager::SetLocalGame(bool isLocal)
{
    m_bIsOfflineGame = isLocal;
    return true;
}

void CSteamNetworkManager::SetPrivateGame(bool isPrivate)
{
    app.DebugPrintf("[Steam] Setting as private game: %s\n", isPrivate ? "yes" : "no");
    m_bIsPrivateGame = isPrivate;
}

// ---- Callbacks ----

void CSteamNetworkManager::RegisterPlayerChangedCallback(int iPad, void (*callback)(void *callbackParam, INetworkPlayer *pPlayer, bool leaving), void *callbackParam)
{
    playerChangedCallback[iPad] = callback;
    playerChangedCallbackParam[iPad] = callbackParam;
}

void CSteamNetworkManager::UnRegisterPlayerChangedCallback(int iPad, void (*callback)(void *callbackParam, INetworkPlayer *pPlayer, bool leaving), void *callbackParam)
{
    if (playerChangedCallbackParam[iPad] == callbackParam)
    {
        playerChangedCallback[iPad] = NULL;
        playerChangedCallbackParam[iPad] = NULL;
    }
}

void CSteamNetworkManager::HandleSignInChange()
{
}

// ---- Network Game ----

bool CSteamNetworkManager::_RunNetworkGame()
{
    extern QNET_STATE _iQNetStubState;
    _iQNetStubState = QNET_STATE_GAME_PLAY;

    for (DWORD i = 0; i < IQNet::s_playerCount; i++)
    {
        if (IQNet::m_player[i].m_isRemote)
        {
            INetworkPlayer *pNetworkPlayer = getNetworkPlayer(&IQNet::m_player[i]);
            if (pNetworkPlayer != NULL && pNetworkPlayer->GetSocket() != NULL)
            {
                Socket::addIncomingSocket(pNetworkPlayer->GetSocket());
            }
        }
    }
    return true;
}

// ---- Session Data ----

void CSteamNetworkManager::UpdateAndSetGameSessionData(INetworkPlayer *pNetworkPlayerLeaving)
{
    // Update lobby metadata if we have an active lobby
    if (m_currentLobby.IsValid() && m_pIQNet->IsHost() && SteamManager::IsInitialized())
    {
        char buf[64];
        sprintf_s(buf, "%d", m_pIQNet->GetPlayerCount());
        SteamMatchmaking()->SetLobbyData(m_currentLobby, "player_count", buf);

        sprintf_s(buf, "%d", MINECRAFT_NET_MAX_PLAYERS);
        SteamMatchmaking()->SetLobbyData(m_currentLobby, "max_players", buf);
    }
}

// ---- Player Notifications ----

void CSteamNetworkManager::NotifyPlayerJoined(IQNetPlayer *pQNetPlayer)
{
    const char *pszDescription;
    bool createFakeSocket = false;
    bool localPlayer = false;

    NetworkPlayerXbox *networkPlayer = (NetworkPlayerXbox *)addNetworkPlayer(pQNetPlayer);

    if (pQNetPlayer->IsLocal())
    {
        localPlayer = true;
        if (pQNetPlayer->IsHost())
        {
            pszDescription = "local host";
            m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
        }
        else
        {
            pszDescription = "local";
            createFakeSocket = true;
        }
    }
    else
    {
        if (pQNetPlayer->IsHost())
        {
            pszDescription = "remote host";
        }
        else
        {
            pszDescription = "remote";
            if (m_pIQNet->IsHost())
                createFakeSocket = true;
        }

        if (m_pIQNet->IsHost() && !m_bHostChanged)
        {
            bool systemHasPrimaryPlayer = false;
            for (auto it = m_machineQNetPrimaryPlayers.begin(); it < m_machineQNetPrimaryPlayers.end(); ++it)
            {
                IQNetPlayer *pQNetPrimaryPlayer = *it;
                if (pQNetPlayer->IsSameSystem(pQNetPrimaryPlayer))
                {
                    systemHasPrimaryPlayer = true;
                    break;
                }
            }
            if (!systemHasPrimaryPlayer)
                m_machineQNetPrimaryPlayers.push_back(pQNetPlayer);
        }
    }

    g_NetworkManager.PlayerJoining(networkPlayer);

    if (createFakeSocket && !m_bHostChanged)
    {
        g_NetworkManager.CreateSocket(networkPlayer, localPlayer);
    }

    app.DebugPrintf("[Steam] Player 0x%p \"%ls\" joined; %s\n",
                    pQNetPlayer, pQNetPlayer->GetGamertag(), pszDescription);

    if (m_pIQNet->IsHost())
    {
        SystemFlagAddPlayer(networkPlayer);
    }

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
    {
        if (playerChangedCallback[idx] != NULL)
            playerChangedCallback[idx](playerChangedCallbackParam[idx], networkPlayer, false);
    }
}

void CSteamNetworkManager::NotifyPlayerLeaving(IQNetPlayer *pQNetPlayer)
{
    app.DebugPrintf("[Steam] Player 0x%p \"%ls\" leaving.\n", pQNetPlayer, pQNetPlayer->GetGamertag());

    INetworkPlayer *networkPlayer = getNetworkPlayer(pQNetPlayer);
    if (networkPlayer == NULL)
        return;

    Socket *socket = networkPlayer->GetSocket();
    if (socket != NULL)
    {
        if (m_pIQNet->IsHost())
            g_NetworkManager.CloseConnection(networkPlayer);
    }

    if (m_pIQNet->IsHost())
    {
        SystemFlagRemovePlayer(networkPlayer);
    }

    g_NetworkManager.PlayerLeaving(networkPlayer);

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
    {
        if (playerChangedCallback[idx] != NULL)
            playerChangedCallback[idx](playerChangedCallbackParam[idx], networkPlayer, true);
    }

    removeNetworkPlayer(pQNetPlayer);
}

// ---- Player Tracking ----

INetworkPlayer *CSteamNetworkManager::addNetworkPlayer(IQNetPlayer *pQNetPlayer)
{
    NetworkPlayerXbox *pNetworkPlayer = new NetworkPlayerXbox(pQNetPlayer);
    pQNetPlayer->SetCustomDataValue((ULONG_PTR)pNetworkPlayer);
    currentNetworkPlayers.push_back(pNetworkPlayer);
    return pNetworkPlayer;
}

void CSteamNetworkManager::removeNetworkPlayer(IQNetPlayer *pQNetPlayer)
{
    INetworkPlayer *pNetworkPlayer = getNetworkPlayer(pQNetPlayer);
    for (auto it = currentNetworkPlayers.begin(); it != currentNetworkPlayers.end(); it++)
    {
        if (*it == pNetworkPlayer)
        {
            currentNetworkPlayers.erase(it);
            return;
        }
    }
}

INetworkPlayer *CSteamNetworkManager::getNetworkPlayer(IQNetPlayer *pQNetPlayer)
{
    return pQNetPlayer ? (INetworkPlayer *)(pQNetPlayer->GetCustomDataValue()) : NULL;
}

bool CSteamNetworkManager::RemoveLocalPlayer(INetworkPlayer *pNetworkPlayer)
{
    return true;
}

// ---- System Flags ----

CSteamNetworkManager::PlayerFlags::PlayerFlags(INetworkPlayer *pNetworkPlayer, unsigned int count)
{
    count = (count + 8 - 1) & ~(8 - 1);
    this->m_pNetworkPlayer = pNetworkPlayer;
    this->flags = new unsigned char[count / 8];
    memset(this->flags, 0, count / 8);
    this->count = count;
}

CSteamNetworkManager::PlayerFlags::~PlayerFlags()
{
    delete[] flags;
}

void CSteamNetworkManager::SystemFlagAddPlayer(INetworkPlayer *pNetworkPlayer)
{
    PlayerFlags *newPlayerFlags = new PlayerFlags(pNetworkPlayer, m_flagIndexSize);
    for (unsigned int i = 0; i < m_playerFlags.size(); i++)
    {
        if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer))
        {
            memcpy(newPlayerFlags->flags, m_playerFlags[i]->flags, m_playerFlags[i]->count / 8);
            break;
        }
    }
    m_playerFlags.push_back(newPlayerFlags);
}

void CSteamNetworkManager::SystemFlagRemovePlayer(INetworkPlayer *pNetworkPlayer)
{
    for (unsigned int i = 0; i < m_playerFlags.size(); i++)
    {
        if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer)
        {
            delete m_playerFlags[i];
            m_playerFlags[i] = m_playerFlags.back();
            m_playerFlags.pop_back();
            return;
        }
    }
}

void CSteamNetworkManager::SystemFlagReset()
{
    for (unsigned int i = 0; i < m_playerFlags.size(); i++)
        delete m_playerFlags[i];
    m_playerFlags.clear();
}

void CSteamNetworkManager::SystemFlagSet(INetworkPlayer *pNetworkPlayer, int index)
{
    if (index < 0 || index >= m_flagIndexSize) return;
    if (pNetworkPlayer == NULL) return;

    for (unsigned int i = 0; i < m_playerFlags.size(); i++)
    {
        if (pNetworkPlayer->IsSameSystem(m_playerFlags[i]->m_pNetworkPlayer))
        {
            m_playerFlags[i]->flags[index / 8] |= (128 >> (index % 8));
        }
    }
}

bool CSteamNetworkManager::SystemFlagGet(INetworkPlayer *pNetworkPlayer, int index)
{
    if (index < 0 || index >= m_flagIndexSize) return false;
    if (pNetworkPlayer == NULL) return false;

    for (unsigned int i = 0; i < m_playerFlags.size(); i++)
    {
        if (m_playerFlags[i]->m_pNetworkPlayer == pNetworkPlayer)
        {
            return ((m_playerFlags[i]->flags[index / 8] & (128 >> (index % 8))) != 0);
        }
    }
    return false;
}

// ---- Stats ----

std::wstring CSteamNetworkManager::GatherStats()
{
    return L"";
}

std::wstring CSteamNetworkManager::GatherRTTStats()
{
    std::wstring stats(L"Rtt: ");
    wchar_t stat[32];

    for (unsigned int i = 0; i < GetPlayerCount(); ++i)
    {
        IQNetPlayer *pQNetPlayer = ((NetworkPlayerXbox *)GetPlayerByIndex(i))->GetQNetPlayer();
        if (!pQNetPlayer->IsLocal())
        {
            ZeroMemory(stat, 32 * sizeof(WCHAR));
            swprintf(stat, 32, L"%d: %d/", i, pQNetPlayer->GetCurrentRtt());
            stats.append(stat);
        }
    }
    return stats;
}

// ---- Session Texture Packs ----

void CSteamNetworkManager::SetSessionTexturePackParentId(int id)
{
    m_hostGameSessionData.texturePackParentId = id;
}

void CSteamNetworkManager::SetSessionSubTexturePackId(int id)
{
    m_hostGameSessionData.subTexturePackId = id;
}

void CSteamNetworkManager::Notify(int ID, ULONG_PTR Param)
{
}

// ---- Session Search / Discovery ----

void CSteamNetworkManager::TickSearch()
{
    if (m_SessionsUpdatedCallback == NULL)
        return;

    static DWORD lastSearchTime = 0;
    DWORD now = GetTickCount();
    if (now - lastSearchTime < 2000)
        return;
    lastSearchTime = now;

    RefreshSessionList();
}

void CSteamNetworkManager::RefreshSessionList()
{
    // First gather LAN sessions (same as stub)
    std::vector<Win64LANSession> lanSessions = WinsockNetLayer::GetDiscoveredSessions();

    for (size_t i = 0; i < friendsSessions[0].size(); i++)
        delete friendsSessions[0][i];
    friendsSessions[0].clear();

    // Add LAN sessions
    for (size_t i = 0; i < lanSessions.size(); i++)
    {
        FriendSessionInfo *info = new FriendSessionInfo();
        size_t nameLen = wcslen(lanSessions[i].hostName);
        info->displayLabel = new wchar_t[nameLen + 1];
        wcscpy_s(info->displayLabel, nameLen + 1, lanSessions[i].hostName);
        info->displayLabelLength = (unsigned char)nameLen;
        info->displayLabelViewableStartIndex = 0;

        info->data.netVersion = lanSessions[i].netVersion;
        info->data.m_uiGameHostSettings = lanSessions[i].gameHostSettings;
        info->data.texturePackParentId = lanSessions[i].texturePackParentId;
        info->data.subTexturePackId = lanSessions[i].subTexturePackId;
        info->data.isReadyToJoin = lanSessions[i].isJoinable;
        info->data.isJoinable = lanSessions[i].isJoinable;
        strncpy_s(info->data.hostIP, sizeof(info->data.hostIP), lanSessions[i].hostIP, _TRUNCATE);
        info->data.hostPort = lanSessions[i].hostPort;
        wcsncpy_s(info->data.hostName, XUSER_NAME_SIZE, lanSessions[i].hostName, _TRUNCATE);
        info->data.playerCount = lanSessions[i].playerCount;
        info->data.maxPlayers = lanSessions[i].maxPlayers;

        info->sessionId = (SessionID)((unsigned __int64)inet_addr(lanSessions[i].hostIP) | ((unsigned __int64)lanSessions[i].hostPort << 32));

        friendsSessions[0].push_back(info);
    }

    // Add Steam lobby sessions from cached results
    for (size_t i = 0; i < m_steamSessions.size(); i++)
    {
        SteamSessionInfo &ss = m_steamSessions[i];
        FriendSessionInfo *info = new FriendSessionInfo();

        size_t nameLen = wcslen(ss.friendInfo.data.hostName);
        info->displayLabel = new wchar_t[nameLen + 16]; // extra for "[Steam] " prefix
        swprintf_s(info->displayLabel, nameLen + 16, L"[Steam] %ls", ss.friendInfo.data.hostName);
        info->displayLabelLength = (unsigned char)wcslen(info->displayLabel);
        info->displayLabelViewableStartIndex = 0;

        info->data = ss.friendInfo.data;
        info->sessionId = ss.lobbyId.ConvertToUint64();

        friendsSessions[0].push_back(info);
    }

    m_searchResultsCount[0] = (int)friendsSessions[0].size();

    if (m_SessionsUpdatedCallback != NULL)
        m_SessionsUpdatedCallback(m_pSearchParam);
}

std::vector<FriendSessionInfo *> *CSteamNetworkManager::GetSessionList(int iPad, int localPlayers, bool partyOnly)
{
    std::vector<FriendSessionInfo *> *filteredList = new std::vector<FriendSessionInfo *>();
    for (size_t i = 0; i < friendsSessions[0].size(); i++)
        filteredList->push_back(friendsSessions[0][i]);

    // Also request a Steam lobby list refresh if Steam is available (Task 4)
    if (SteamManager::IsInitialized())
    {
        SteamMatchmaking()->AddRequestLobbyListDistanceFilter(k_ELobbyDistanceFilterWorldwide);
        SteamMatchmaking()->AddRequestLobbyListStringFilter("game", "minecraft_console", k_ELobbyComparisonEqual);
        SteamAPICall_t hCall = SteamMatchmaking()->RequestLobbyList();
        m_callResultLobbyList.Set(hCall, this, &CSteamNetworkManager::OnLobbyMatchList);
    }

    return filteredList;
}

bool CSteamNetworkManager::GetGameSessionInfo(int iPad, SessionID sessionId, FriendSessionInfo *foundSessionInfo)
{
    return false;
}

void CSteamNetworkManager::SetSessionsUpdatedCallback(void (*SessionsUpdatedCallback)(LPVOID pParam), LPVOID pSearchParam)
{
    m_SessionsUpdatedCallback = SessionsUpdatedCallback;
    m_pSearchParam = pSearchParam;
}

void CSteamNetworkManager::GetFullFriendSessionInfo(FriendSessionInfo *foundSession, void (*FriendSessionUpdatedFn)(bool success, void *pParam), void *pParam)
{
    FriendSessionUpdatedFn(true, pParam);
}

void CSteamNetworkManager::ForceFriendsSessionRefresh()
{
    app.DebugPrintf("[Steam] Resetting friends session search data\n");
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        m_searchResultsCount[i] = 0;
    }
    m_steamSessions.clear();
}

// ---- Steam Lobby Callbacks (Task 4) ----

void CSteamNetworkManager::OnLobbyCreated(LobbyCreated_t *pResult, bool bIOFailure)
{
    if (bIOFailure || pResult->m_eResult != k_EResultOK)
    {
        app.DebugPrintf("[Steam] Failed to create lobby (result=%d, ioFail=%d)\n",
                        pResult ? pResult->m_eResult : -1, bIOFailure);
        return;
    }

    m_currentLobby = CSteamID(pResult->m_ulSteamIDLobby);
    m_bLobbyCreated = true;

    // Set lobby metadata for discoverability
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "game", "minecraft_console");
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "version", "1.0");

    // Set host IP and port so joiners know where to connect
    char portStr[16];
    sprintf_s(portStr, "%d", WinsockNetLayer::GetHostPort());
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "host_port", portStr);

    // Set host name
    char hostNameUTF8[128];
    WideCharToMultiByte(CP_UTF8, 0, IQNet::m_player[0].m_gamertag, -1,
                        hostNameUTF8, sizeof(hostNameUTF8), NULL, NULL);
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "host_name", hostNameUTF8);

    char buf[16];
    sprintf_s(buf, "%d", (int)IQNet::s_playerCount);
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "player_count", buf);
    sprintf_s(buf, "%d", MINECRAFT_NET_MAX_PLAYERS);
    SteamMatchmaking()->SetLobbyData(m_currentLobby, "max_players", buf);

    app.DebugPrintf("[Steam] Lobby created: %llu\n", m_currentLobby.ConvertToUint64());
}

void CSteamNetworkManager::OnLobbyMatchList(LobbyMatchList_t *pResult, bool bIOFailure)
{
    if (bIOFailure || pResult == NULL)
    {
        app.DebugPrintf("[Steam] Lobby list request failed\n");
        return;
    }

    m_steamSessions.clear();

    for (uint32 i = 0; i < pResult->m_nLobbiesMatching; i++)
    {
        CSteamID lobbyId = SteamMatchmaking()->GetLobbyByIndex(i);
        const char *gameTag = SteamMatchmaking()->GetLobbyData(lobbyId, "game");

        if (!gameTag || strcmp(gameTag, "minecraft_console") != 0)
            continue;

        SteamSessionInfo ss;
        ss.lobbyId = lobbyId;
        memset(&ss.friendInfo, 0, sizeof(ss.friendInfo));

        const char *hostPort = SteamMatchmaking()->GetLobbyData(lobbyId, "host_port");
        const char *hostName = SteamMatchmaking()->GetLobbyData(lobbyId, "host_name");
        const char *playerCount = SteamMatchmaking()->GetLobbyData(lobbyId, "player_count");
        const char *maxPlayers = SteamMatchmaking()->GetLobbyData(lobbyId, "max_players");

        if (hostPort) ss.friendInfo.data.hostPort = atoi(hostPort);
        if (playerCount) ss.friendInfo.data.playerCount = (unsigned char)atoi(playerCount);
        if (maxPlayers) ss.friendInfo.data.maxPlayers = (unsigned char)atoi(maxPlayers);
        ss.friendInfo.data.isJoinable = true;
        ss.friendInfo.data.isReadyToJoin = true;

        if (hostName)
        {
            MultiByteToWideChar(CP_UTF8, 0, hostName, -1,
                                ss.friendInfo.data.hostName, XUSER_NAME_SIZE);
        }
        else
        {
            wcscpy_s(ss.friendInfo.data.hostName, XUSER_NAME_SIZE, L"Steam Game");
        }

        m_steamSessions.push_back(ss);
    }

    app.DebugPrintf("[Steam] Found %d lobbies\n", (int)m_steamSessions.size());

    // Trigger a session list refresh
    if (m_SessionsUpdatedCallback != NULL)
        m_SessionsUpdatedCallback(m_pSearchParam);
}

void CSteamNetworkManager::OnLobbyEnter(LobbyEnter_t *pResult, bool bIOFailure)
{
    if (bIOFailure || pResult == NULL || pResult->m_EChatRoomEnterResponse != 1)
    {
        app.DebugPrintf("[Steam] Failed to enter lobby\n");
        return;
    }

    m_currentLobby = CSteamID(pResult->m_ulSteamIDLobby);

    // Get host connection info from lobby data
    const char *hostPort = SteamMatchmaking()->GetLobbyData(m_currentLobby, "host_port");
    // Note: In a full implementation, the host's IP would be obtained via
    // Steam relay or P2P. For now, the lobby stores direct connection info.

    app.DebugPrintf("[Steam] Entered lobby %llu\n", m_currentLobby.ConvertToUint64());
}

// ---- Steam Networking Connection Status Callback ----

void CSteamNetworkManager::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pParam)
{
    if (pParam == NULL)
        return;

    switch (pParam->m_info.m_eState)
    {
    case k_ESteamNetworkingConnectionState_Connecting:
        // Incoming connection - accept it
        if (m_pSteamNetSockets && m_pIQNet->IsHost())
        {
            m_pSteamNetSockets->AcceptConnection(pParam->m_hConn);
            if (m_pollGroup != k_HSteamNetPollGroup_Invalid)
                m_pSteamNetSockets->SetConnectionPollGroup(pParam->m_hConn, m_pollGroup);
            app.DebugPrintf("[Steam] Accepted incoming connection %u\n", pParam->m_hConn);
        }
        break;

    case k_ESteamNetworkingConnectionState_Connected:
        app.DebugPrintf("[Steam] Connection %u established\n", pParam->m_hConn);
        break;

    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        app.DebugPrintf("[Steam] Connection %u closed: %s\n",
                        pParam->m_hConn, pParam->m_info.m_szEndDebug);
        if (m_pSteamNetSockets)
            m_pSteamNetSockets->CloseConnection(pParam->m_hConn, 0, NULL, false);
        break;

    default:
        break;
    }
}
