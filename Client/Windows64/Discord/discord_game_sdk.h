// ============================================================================
// STUB/PLACEHOLDER - Discord Game SDK C API Header
// Replace this file with the real discord_game_sdk.h from the Discord Developer Portal.
// https://discord.com/developers/docs/game-sdk/sdk-starter-guide
// ============================================================================
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version
#define DISCORD_VERSION 3
#define DISCORD_APPLICATION_MANAGER_VERSION 1
#define DISCORD_USER_MANAGER_VERSION 1
#define DISCORD_IMAGE_MANAGER_VERSION 1
#define DISCORD_ACTIVITY_MANAGER_VERSION 1
#define DISCORD_RELATIONSHIP_MANAGER_VERSION 1
#define DISCORD_LOBBY_MANAGER_VERSION 1
#define DISCORD_NETWORK_MANAGER_VERSION 1
#define DISCORD_OVERLAY_MANAGER_VERSION 2
#define DISCORD_STORAGE_MANAGER_VERSION 1
#define DISCORD_STORE_MANAGER_VERSION 1
#define DISCORD_VOICE_MANAGER_VERSION 1
#define DISCORD_ACHIEVEMENT_MANAGER_VERSION 1

// Result codes
enum EDiscordResult {
    DiscordResult_Ok = 0,
    DiscordResult_ServiceUnavailable = 1,
    DiscordResult_InvalidVersion = 2,
    DiscordResult_LockFailed = 3,
    DiscordResult_InternalError = 4,
    DiscordResult_InvalidPayload = 5,
    DiscordResult_InvalidCommand = 6,
    DiscordResult_InvalidPermissions = 7,
    DiscordResult_NotFetched = 8,
    DiscordResult_NotFound = 9,
    DiscordResult_Conflict = 10,
    DiscordResult_InvalidSecret = 11,
    DiscordResult_InvalidJoinSecret = 12,
    DiscordResult_NoEligibleActivity = 13,
    DiscordResult_InvalidInvite = 14,
    DiscordResult_NotAuthenticated = 15,
    DiscordResult_InvalidAccessToken = 16,
    DiscordResult_ApplicationMismatch = 17,
    DiscordResult_InvalidDataUrl = 18,
    DiscordResult_InvalidBase64 = 19,
    DiscordResult_NotFiltered = 20,
    DiscordResult_LobbyFull = 21,
    DiscordResult_InvalidLobbySecret = 22,
    DiscordResult_InvalidFilename = 23,
    DiscordResult_InvalidFileSize = 24,
    DiscordResult_InvalidEntitlement = 25,
    DiscordResult_NotInstalled = 26,
    DiscordResult_NotRunning = 27,
    DiscordResult_InsufficientBuffer = 28,
    DiscordResult_PurchaseCanceled = 29,
    DiscordResult_InvalidGuild = 30,
    DiscordResult_InvalidEvent = 31,
    DiscordResult_InvalidChannel = 32,
    DiscordResult_InvalidOrigin = 33,
    DiscordResult_RateLimited = 34,
    DiscordResult_OAuth2Error = 35,
    DiscordResult_SelectChannelTimeout = 36,
    DiscordResult_GetGuildTimeout = 37,
    DiscordResult_SelectVoiceForceRequired = 38,
    DiscordResult_CaptureShortcutAlreadyListening = 39,
    DiscordResult_UnauthorizedForAchievement = 40,
    DiscordResult_InvalidGiftCode = 41,
    DiscordResult_PurchaseError = 42,
    DiscordResult_TransactionAborted = 43,
};

// Create flags
enum EDiscordCreateFlags {
    DiscordCreateFlags_Default = 0,
    DiscordCreateFlags_NoRequireDiscord = 1,
};

// Activity types
enum EDiscordActivityType {
    DiscordActivityType_Playing = 0,
    DiscordActivityType_Streaming = 1,
    DiscordActivityType_Listening = 2,
    DiscordActivityType_Watching = 3,
};

enum EDiscordActivityActionType {
    DiscordActivityActionType_Join = 1,
    DiscordActivityActionType_Spectate = 2,
};

enum EDiscordActivityJoinRequestReply {
    DiscordActivityJoinRequestReply_No = 0,
    DiscordActivityJoinRequestReply_Yes = 1,
    DiscordActivityJoinRequestReply_Ignore = 2,
};

// Structs
typedef int64_t DiscordClientId;
typedef int64_t DiscordSnowflake;
typedef int64_t DiscordTimestamp;
typedef uint32_t DiscordVersion;

struct DiscordUser {
    DiscordSnowflake id;
    char username[256];
    char discriminator[8];
    char avatar[128];
    bool bot;
};

struct DiscordActivityTimestamps {
    DiscordTimestamp start;
    DiscordTimestamp end;
};

struct DiscordActivityAssets {
    char large_image[128];
    char large_text[128];
    char small_image[128];
    char small_text[128];
};

struct DiscordPartySize {
    int32_t current_size;
    int32_t max_size;
};

struct DiscordActivityParty {
    char id[128];
    struct DiscordPartySize size;
};

struct DiscordActivitySecrets {
    char match[128];
    char join[128];
    char spectate[128];
};

struct DiscordActivity {
    enum EDiscordActivityType type;
    int64_t application_id;
    char name[128];
    char state[128];
    char details[128];
    struct DiscordActivityTimestamps timestamps;
    struct DiscordActivityAssets assets;
    struct DiscordActivityParty party;
    struct DiscordActivitySecrets secrets;
    bool instance;
};

// Callback typedefs
typedef void (*IDiscordActivityEvents_OnActivityJoin)(void* event_data, const char* secret);
typedef void (*IDiscordActivityEvents_OnActivitySpectate)(void* event_data, const char* secret);
typedef void (*IDiscordActivityEvents_OnActivityJoinRequest)(void* event_data, struct DiscordUser* user);
typedef void (*IDiscordActivityEvents_OnActivityInvite)(void* event_data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity);

struct IDiscordActivityEvents {
    IDiscordActivityEvents_OnActivityJoin on_activity_join;
    IDiscordActivityEvents_OnActivitySpectate on_activity_spectate;
    IDiscordActivityEvents_OnActivityJoinRequest on_activity_join_request;
    IDiscordActivityEvents_OnActivityInvite on_activity_invite;
};

// Manager interfaces (vtable-style C API)

typedef void (*DiscordResultCallback)(void* callback_data, enum EDiscordResult result);
typedef void (*DiscordUserCallback)(void* callback_data, enum EDiscordResult result, struct DiscordUser* user);

struct IDiscordActivityManager {
    enum EDiscordResult (*register_command)(struct IDiscordActivityManager* manager, const char* command);
    enum EDiscordResult (*register_steam)(struct IDiscordActivityManager* manager, uint32_t steam_id);
    void (*update_activity)(struct IDiscordActivityManager* manager, struct DiscordActivity* activity, void* callback_data, DiscordResultCallback callback);
    void (*clear_activity)(struct IDiscordActivityManager* manager, void* callback_data, DiscordResultCallback callback);
    void (*send_request_reply)(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, enum EDiscordActivityJoinRequestReply reply, void* callback_data, DiscordResultCallback callback);
    void (*send_invite)(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, enum EDiscordActivityActionType type, const char* content, void* callback_data, DiscordResultCallback callback);
    void (*accept_invite)(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, void* callback_data, DiscordResultCallback callback);
};

struct IDiscordUserManager {
    void (*get_current_user)(struct IDiscordUserManager* manager, struct DiscordUser* user);
    void (*get_user)(struct IDiscordUserManager* manager, DiscordSnowflake user_id, void* callback_data, DiscordUserCallback callback);
    enum EDiscordResult (*get_current_user_premium_type)(struct IDiscordUserManager* manager, int32_t* premium_type);
    enum EDiscordResult (*current_user_has_flag)(struct IDiscordUserManager* manager, int32_t flag, bool* has_flag);
};

// Overlay manager interface
struct IDiscordOverlayManager {
    void (*is_enabled)(struct IDiscordOverlayManager* manager, bool* enabled);
    void (*is_locked)(struct IDiscordOverlayManager* manager, bool* locked);
    void (*set_locked)(struct IDiscordOverlayManager* manager, bool locked, void* callback_data, DiscordResultCallback callback);
    void (*open_activity_invite)(struct IDiscordOverlayManager* manager, enum EDiscordActivityActionType type, void* callback_data, DiscordResultCallback callback);
    void (*open_guild_invite)(struct IDiscordOverlayManager* manager, const char* code, void* callback_data, DiscordResultCallback callback);
    void (*open_voice_settings)(struct IDiscordOverlayManager* manager, void* callback_data, DiscordResultCallback callback);
};

// Core interface
struct IDiscordCore {
    void (*destroy)(struct IDiscordCore* core);
    enum EDiscordResult (*run_callbacks)(struct IDiscordCore* core);
    void (*set_log_hook)(struct IDiscordCore* core, int min_level, void* hook_data, void (*hook)(void* hook_data, int level, const char* message));
    struct IDiscordActivityManager* (*get_activity_manager)(struct IDiscordCore* core);
    struct IDiscordUserManager* (*get_user_manager)(struct IDiscordCore* core);
    struct IDiscordOverlayManager* (*get_overlay_manager)(struct IDiscordCore* core);
};

// Create params
struct DiscordCreateParams {
    DiscordClientId client_id;
    uint64_t flags;
    struct IDiscordActivityEvents* activity_events;
    void* event_data;
};

// Helper to set defaults
static inline void DiscordCreateParamsSetDefault(struct DiscordCreateParams* params) {
    params->client_id = 0;
    params->flags = DiscordCreateFlags_Default;
    params->activity_events = NULL;
    params->event_data = NULL;
}

// Main entry point - loaded from discord_game_sdk.dll
// STUB: In real SDK, this is exported from the DLL.
// We declare it here; the implementation is in discord_game_sdk_stub.cpp
enum EDiscordResult DiscordCreate(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result);

#ifdef __cplusplus
}
#endif
