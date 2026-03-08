#pragma once

#include <string>
#include <Windows.h>
#include "../../Windows64/Discord/discord_game_sdk.h"

class DiscordManager
{
public:
    static DiscordManager& Get();

    bool Initialize(int64_t applicationId);
    void Shutdown();
    void Tick(); // Call every frame - runs callbacks

    // Rich Presence
    void SetPresence(const wchar_t* state, const wchar_t* details,
                     const char* largeImageKey = "default",
                     int partySize = 0, int partyMax = 0);
    void ClearPresence();

    // Dimension-aware presence updates
    void UpdateDimensionPresence(int dimensionId, const wchar_t* levelName);
    void UpdateGameModePresence(const wchar_t* gameModeName);
    void UpdatePlayerCount(int currentPlayers, int maxPlayers);
    void SetPausedPresence();
    void SetLoadingPresence(const wchar_t* statusText = nullptr);
    void ResumeGamePresence();

    // Overlay
    void OpenOverlay();
    void OpenActivityInviteDialog();
    bool IsOverlayEnabled() const { return m_overlayEnabled; }

    // User info
    bool IsConnected() const { return m_connected; }
    int64_t GetCurrentUserId() const { return m_currentUserId; }
    const char* GetCurrentUsername() const { return m_username; }

    // Activity (join/invite)
    void SetJoinSecret(const char* secret);
    void SendInvite(int64_t targetUserId);

    // Callbacks
    typedef void (*JoinCallback)(const char* secret);
    void SetJoinCallback(JoinCallback cb) { m_joinCallback = cb; }

    // Pending join data (for main thread processing)
    bool HasPendingJoin();
    void GetPendingJoinSecret(char* outBuf, int outBufLen);
    void ClearPendingJoin();

    // Parse a join secret string "MC|<ip>|<port>|<sessionId>"
    // Returns true if parsing succeeded.
    static bool ParseJoinSecret(const char* secret, char* ipOut, int ipMaxLen,
                                int* portOut, unsigned long long* sessionIdOut);

    // Get dimension name for Discord image key
    static const char* GetDimensionImageKey(int dimensionId);
    static const wchar_t* GetDimensionDisplayName(int dimensionId);

private:
    DiscordManager();
    ~DiscordManager();
    DiscordManager(const DiscordManager&) = delete;
    DiscordManager& operator=(const DiscordManager&) = delete;

    struct IDiscordCore* m_core;
    struct IDiscordActivityManager* m_activities;
    struct IDiscordUserManager* m_users;
    struct IDiscordOverlayManager* m_overlay;

    volatile bool m_connected;
    bool m_initialized;
    int64_t m_currentUserId;
    char m_username[256];

    // Current activity state (cached for updates)
    struct DiscordActivity m_currentActivity;

    // Cached game state for re-composing presence
    int m_currentDimension;
    wchar_t m_currentLevelName[128];
    wchar_t m_currentGameMode[32];
    int m_currentPartySize;
    int m_currentPartyMax;
    bool m_isPaused;
    bool m_isLoading;
    bool m_overlayEnabled;

    JoinCallback m_joinCallback;

    // Pending join from Discord callback (processed on main thread)
    // Protected by m_pendingJoinCS for thread safety
    CRITICAL_SECTION m_pendingJoinCS;
    volatile bool m_pendingJoin;
    char m_pendingJoinSecret[128];

    // Activity event handlers storage
    struct IDiscordActivityEvents m_activityEvents;

    // Rebuild and push the current activity from cached state
    void RebuildPresence();

    // Static callbacks for Discord SDK
    static void OnActivityJoin(void* data, const char* secret);
    static void OnActivityInvite(void* data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity);
};
