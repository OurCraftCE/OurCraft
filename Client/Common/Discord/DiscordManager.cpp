#include "stdafx.h"
#include "DiscordManager.h"
#include <stdio.h>
#include <string.h>

// Forward declare for DebugPrintf logging
extern class CConsoleMinecraftApp app;

DiscordManager& DiscordManager::Get()
{
    static DiscordManager instance;
    return instance;
}

DiscordManager::DiscordManager()
    : m_core(nullptr)
    , m_activities(nullptr)
    , m_users(nullptr)
    , m_overlay(nullptr)
    , m_connected(false)
    , m_initialized(false)
    , m_currentUserId(0)
    , m_joinCallback(nullptr)
    , m_pendingJoin(false)
    , m_currentDimension(0)
    , m_currentPartySize(0)
    , m_currentPartyMax(0)
    , m_isPaused(false)
    , m_isLoading(false)
    , m_overlayEnabled(false)
{
    memset(m_username, 0, sizeof(m_username));
    memset(&m_currentActivity, 0, sizeof(m_currentActivity));
    memset(m_pendingJoinSecret, 0, sizeof(m_pendingJoinSecret));
    memset(&m_activityEvents, 0, sizeof(m_activityEvents));
    memset(m_currentLevelName, 0, sizeof(m_currentLevelName));
    memset(m_currentGameMode, 0, sizeof(m_currentGameMode));
    InitializeCriticalSection(&m_pendingJoinCS);
}

DiscordManager::~DiscordManager()
{
    Shutdown();
    DeleteCriticalSection(&m_pendingJoinCS);
}

bool DiscordManager::Initialize(int64_t applicationId)
{
    if (m_initialized)
    {
        return m_connected;
    }

    m_initialized = true; // Mark early so we don't retry on failure

    struct DiscordCreateParams params;
    DiscordCreateParamsSetDefault(&params);
    params.client_id = applicationId;
    params.flags = DiscordCreateFlags_NoRequireDiscord;

    // Set up activity event handlers
    memset(&m_activityEvents, 0, sizeof(m_activityEvents));
    m_activityEvents.on_activity_join = OnActivityJoin;
    m_activityEvents.on_activity_invite = OnActivityInvite;
    params.activity_events = &m_activityEvents;
    params.event_data = this;

    enum EDiscordResult result = DiscordResult_Ok;

    __try
    {
        result = DiscordCreate(DISCORD_VERSION, &params, &m_core);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        app.DebugPrintf("[Discord] Exception during DiscordCreate - Discord SDK crashed.\n");
        m_core = nullptr;
        m_connected = false;
        return false;
    }

    if (result != DiscordResult_Ok)
    {
        if (result == DiscordResult_NotRunning || result == DiscordResult_NotInstalled)
        {
            app.DebugPrintf("[Discord] Discord is not running or not installed. Rich presence disabled.\n");
        }
        else
        {
            app.DebugPrintf("[Discord] Failed to create Discord SDK instance. Error: %d\n", (int)result);
        }
        m_core = nullptr;
        m_connected = false;
        return false;
    }

    m_activities = m_core->get_activity_manager(m_core);
    m_users = m_core->get_user_manager(m_core);
    m_overlay = m_core->get_overlay_manager(m_core);
    m_connected = true;

    // Get current user info
    struct DiscordUser currentUser;
    memset(&currentUser, 0, sizeof(currentUser));
    m_users->get_current_user(m_users, &currentUser);
    m_currentUserId = currentUser.id;
    strncpy(m_username, currentUser.username, sizeof(m_username) - 1);
    m_username[sizeof(m_username) - 1] = '\0';

    // Check overlay availability
    if (m_overlay)
    {
        bool enabled = false;
        m_overlay->is_enabled(m_overlay, &enabled);
        m_overlayEnabled = enabled;
    }

    // Set up log hook to route Discord SDK logs through our debug console
    m_core->set_log_hook(m_core, 1 /* DiscordLogLevel_Info */, nullptr,
        [](void* /*hook_data*/, int level, const char* message) {
            const char* levelStr = "INFO";
            if (level >= 4) levelStr = "ERROR";
            else if (level >= 3) levelStr = "WARN";
            else if (level >= 2) levelStr = "DEBUG";
            app.DebugPrintf("[Discord SDK %s] %s\n", levelStr, message ? message : "(null)");
        });

    app.DebugPrintf("[Discord] Initialized successfully. User: %s (ID: %lld)\n", m_username, (long long)m_currentUserId);

    return true;
}

void DiscordManager::Shutdown()
{
    if (m_core)
    {
        ClearPresence();
        m_core->destroy(m_core);
        m_core = nullptr;
        m_activities = nullptr;
        m_users = nullptr;
        m_overlay = nullptr;
    }
    m_connected = false;
    m_initialized = false;
    m_overlayEnabled = false;
    app.DebugPrintf("[Discord] Shutdown complete.\n");
}

void DiscordManager::Tick()
{
    if (!m_core || !m_connected) return;

    enum EDiscordResult result = DiscordResult_Ok;

    __try
    {
        result = m_core->run_callbacks(m_core);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        app.DebugPrintf("[Discord] Exception in run_callbacks. Disconnecting.\n");
        m_connected = false;
        return;
    }

    if (result != DiscordResult_Ok)
    {
        app.DebugPrintf("[Discord] run_callbacks failed with error: %d. Disconnecting.\n", (int)result);
        m_connected = false;
    }
}

void DiscordManager::SetPresence(const wchar_t* state, const wchar_t* details,
                                  const char* largeImageKey, int partySize, int partyMax)
{
    if (!m_connected || !m_activities) return;

    memset(&m_currentActivity, 0, sizeof(m_currentActivity));

    // Convert wchar_t to UTF-8
    char stateUtf8[128] = {0};
    char detailsUtf8[128] = {0};

    if (state)
    {
        WideCharToMultiByte(CP_UTF8, 0, state, -1, stateUtf8, sizeof(stateUtf8), NULL, NULL);
    }
    if (details)
    {
        WideCharToMultiByte(CP_UTF8, 0, details, -1, detailsUtf8, sizeof(detailsUtf8), NULL, NULL);
    }

    strncpy(m_currentActivity.state, stateUtf8, sizeof(m_currentActivity.state) - 1);
    strncpy(m_currentActivity.details, detailsUtf8, sizeof(m_currentActivity.details) - 1);

    if (largeImageKey)
    {
        strncpy(m_currentActivity.assets.large_image, largeImageKey,
                sizeof(m_currentActivity.assets.large_image) - 1);
    }

    if (partySize > 0)
    {
        m_currentActivity.party.size.current_size = partySize;
        m_currentActivity.party.size.max_size = partyMax;
    }

    m_activities->update_activity(m_activities, &m_currentActivity, NULL, NULL);
}

void DiscordManager::ClearPresence()
{
    if (!m_connected || !m_activities) return;

    m_activities->clear_activity(m_activities, NULL, NULL);
    memset(&m_currentActivity, 0, sizeof(m_currentActivity));
}

void DiscordManager::UpdateDimensionPresence(int dimensionId, const wchar_t* levelName)
{
    if (!m_connected) return;

    m_currentDimension = dimensionId;
    if (levelName)
    {
        wcsncpy(m_currentLevelName, levelName, 127);
        m_currentLevelName[127] = L'\0';
    }
    m_isPaused = false;
    m_isLoading = false;

    RebuildPresence();
}

void DiscordManager::UpdateGameModePresence(const wchar_t* gameModeName)
{
    if (!m_connected) return;

    if (gameModeName)
    {
        wcsncpy(m_currentGameMode, gameModeName, 31);
        m_currentGameMode[31] = L'\0';
    }

    RebuildPresence();
}

void DiscordManager::UpdatePlayerCount(int currentPlayers, int maxPlayers)
{
    if (!m_connected) return;

    m_currentPartySize = currentPlayers;
    m_currentPartyMax = maxPlayers;

    RebuildPresence();
}

void DiscordManager::SetPausedPresence()
{
    if (!m_connected) return;

    m_isPaused = true;
    RebuildPresence();
}

void DiscordManager::SetLoadingPresence(const wchar_t* statusText)
{
    if (!m_connected) return;

    m_isLoading = true;
    if (statusText)
    {
        SetPresence(L"Loading...", statusText, "default");
    }
    else
    {
        SetPresence(L"Loading...", L"Preparing world", "default");
    }
}

void DiscordManager::ResumeGamePresence()
{
    if (!m_connected) return;

    m_isPaused = false;
    m_isLoading = false;
    RebuildPresence();
}

void DiscordManager::RebuildPresence()
{
    if (!m_connected || !m_activities) return;

    // Build the state string: "Playing <GameMode>" or "Paused" or "Loading..."
    wchar_t stateW[128] = {0};
    if (m_isLoading)
    {
        wcscpy(stateW, L"Loading...");
    }
    else if (m_isPaused)
    {
        wcscpy(stateW, L"Paused");
    }
    else if (m_currentPartySize > 1)
    {
        if (m_currentGameMode[0])
            _snwprintf(stateW, 127, L"Playing Multiplayer - %s", m_currentGameMode);
        else
            wcscpy(stateW, L"Playing Multiplayer");
    }
    else
    {
        if (m_currentGameMode[0])
            _snwprintf(stateW, 127, L"Playing Singleplayer - %s", m_currentGameMode);
        else
            wcscpy(stateW, L"Playing Singleplayer");
    }

    // Build details: "<LevelName> - <Dimension>"
    wchar_t detailsW[128] = {0};
    const wchar_t* dimName = GetDimensionDisplayName(m_currentDimension);
    if (m_currentLevelName[0] && dimName)
    {
        _snwprintf(detailsW, 127, L"%s - %s", m_currentLevelName, dimName);
    }
    else if (m_currentLevelName[0])
    {
        wcsncpy(detailsW, m_currentLevelName, 127);
        detailsW[127] = L'\0';
    }

    const char* imageKey = GetDimensionImageKey(m_currentDimension);
    SetPresence(stateW, detailsW, imageKey, m_currentPartySize, m_currentPartyMax);
}

void DiscordManager::OpenOverlay()
{
    if (!m_connected || !m_overlay) return;

    // The overlay open_voice_settings is a general-purpose way to open the overlay.
    // There is no generic "open overlay" call in the SDK; voice settings is a reasonable default.
    m_overlay->open_voice_settings(m_overlay, NULL, NULL);
    app.DebugPrintf("[Discord] Requested overlay open.\n");
}

void DiscordManager::OpenActivityInviteDialog()
{
    if (!m_connected || !m_overlay) return;

    m_overlay->open_activity_invite(m_overlay, DiscordActivityActionType_Join, NULL, NULL);
    app.DebugPrintf("[Discord] Requested activity invite dialog.\n");
}

void DiscordManager::SetJoinSecret(const char* secret)
{
    if (!m_connected || !m_activities) return;

    if (secret)
    {
        strncpy(m_currentActivity.secrets.join, secret,
                sizeof(m_currentActivity.secrets.join) - 1);
        m_currentActivity.secrets.join[sizeof(m_currentActivity.secrets.join) - 1] = '\0';
    }
    else
    {
        memset(m_currentActivity.secrets.join, 0, sizeof(m_currentActivity.secrets.join));
    }

    // Re-push the activity with the updated secret
    m_activities->update_activity(m_activities, &m_currentActivity, NULL, NULL);
}

void DiscordManager::SendInvite(int64_t targetUserId)
{
    if (!m_connected || !m_activities) return;

    m_activities->send_invite(m_activities, targetUserId, DiscordActivityActionType_Join,
                              "Join my Minecraft game!", NULL, NULL);
}

// Static callback: called by Discord SDK when user accepts a join invite
void DiscordManager::OnActivityJoin(void* data, const char* secret)
{
    DiscordManager* self = (DiscordManager*)data;
    if (!self) return;

    app.DebugPrintf("[Discord] OnActivityJoin: secret=%s\n", secret ? secret : "(null)");

    // Store for main thread processing - protected by critical section
    if (secret)
    {
        EnterCriticalSection(&self->m_pendingJoinCS);
        strncpy(self->m_pendingJoinSecret, secret, sizeof(self->m_pendingJoinSecret) - 1);
        self->m_pendingJoinSecret[sizeof(self->m_pendingJoinSecret) - 1] = '\0';
        self->m_pendingJoin = true;
        LeaveCriticalSection(&self->m_pendingJoinCS);
    }

    // Also fire the callback if set
    if (self->m_joinCallback && secret)
    {
        self->m_joinCallback(secret);
    }
}

// Static callback: called by Discord SDK when user receives an invite
void DiscordManager::OnActivityInvite(void* data, enum EDiscordActivityActionType type,
                                       struct DiscordUser* user, struct DiscordActivity* activity)
{
    if (!user) return;

    app.DebugPrintf("[Discord] OnActivityInvite: from user %s (ID: %lld)\n",
           user->username, (long long)user->id);
}

// Thread-safe pending join accessors
bool DiscordManager::HasPendingJoin()
{
    return m_pendingJoin;
}

void DiscordManager::GetPendingJoinSecret(char* outBuf, int outBufLen)
{
    if (!outBuf || outBufLen <= 0) return;

    EnterCriticalSection(&m_pendingJoinCS);
    strncpy(outBuf, m_pendingJoinSecret, outBufLen - 1);
    outBuf[outBufLen - 1] = '\0';
    LeaveCriticalSection(&m_pendingJoinCS);
}

void DiscordManager::ClearPendingJoin()
{
    EnterCriticalSection(&m_pendingJoinCS);
    m_pendingJoin = false;
    memset(m_pendingJoinSecret, 0, sizeof(m_pendingJoinSecret));
    LeaveCriticalSection(&m_pendingJoinCS);
}

bool DiscordManager::ParseJoinSecret(const char* secret, char* ipOut, int ipMaxLen,
                                      int* portOut, unsigned long long* sessionIdOut)
{
    if (!secret || !ipOut || !portOut || !sessionIdOut) return false;

    // Expected format: "MC|<ip>|<port>|<sessionId>"
    int port = 0;
    unsigned long long sessionId = 0;
    char ip[64] = {0};

    int matched = sscanf(secret, "MC|%63[^|]|%d|%llu", ip, &port, &sessionId);
    if (matched != 3)
    {
        app.DebugPrintf("[Discord] Failed to parse join secret: %s\n", secret);
        return false;
    }

    strncpy(ipOut, ip, ipMaxLen - 1);
    ipOut[ipMaxLen - 1] = '\0';
    *portOut = port;
    *sessionIdOut = sessionId;

    app.DebugPrintf("[Discord] Parsed join secret: ip=%s port=%d sessionId=%llu\n", ipOut, *portOut, *sessionIdOut);
    return true;
}

const char* DiscordManager::GetDimensionImageKey(int dimensionId)
{
    switch (dimensionId)
    {
    case -1: return "nether";
    case  1: return "the_end";
    case  0:
    default: return "overworld";
    }
}

const wchar_t* DiscordManager::GetDimensionDisplayName(int dimensionId)
{
    switch (dimensionId)
    {
    case -1: return L"The Nether";
    case  1: return L"The End";
    case  0:
    default: return L"Overworld";
    }
}
