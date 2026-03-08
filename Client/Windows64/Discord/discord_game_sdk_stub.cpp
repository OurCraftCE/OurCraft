// ============================================================================
// STUB/PLACEHOLDER - Discord Game SDK stub implementation
// Replace with real discord_game_sdk.lib/.dll from the Discord Developer Portal.
// This stub allows the project to compile and link without the real SDK.
// All API calls will return failure or no-op gracefully.
// ============================================================================

#include "discord_game_sdk.h"
#include <string.h>
#include <stdio.h>

// Stub activity manager
static enum EDiscordResult stub_register_command(struct IDiscordActivityManager* manager, const char* command) { return DiscordResult_Ok; }
static enum EDiscordResult stub_register_steam(struct IDiscordActivityManager* manager, uint32_t steam_id) { return DiscordResult_Ok; }
static void stub_update_activity(struct IDiscordActivityManager* manager, struct DiscordActivity* activity, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_clear_activity(struct IDiscordActivityManager* manager, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_send_request_reply(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, enum EDiscordActivityJoinRequestReply reply, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_send_invite(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, enum EDiscordActivityActionType type, const char* content, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_accept_invite(struct IDiscordActivityManager* manager, DiscordSnowflake user_id, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}

static struct IDiscordActivityManager s_stubActivityManager = {
    stub_register_command,
    stub_register_steam,
    stub_update_activity,
    stub_clear_activity,
    stub_send_request_reply,
    stub_send_invite,
    stub_accept_invite,
};

// Stub user manager
static void stub_get_current_user(struct IDiscordUserManager* manager, struct DiscordUser* user) {
    memset(user, 0, sizeof(*user));
    user->id = 0;
    strncpy(user->username, "StubUser", sizeof(user->username));
    strncpy(user->discriminator, "0000", sizeof(user->discriminator));
}
static void stub_get_user(struct IDiscordUserManager* manager, DiscordSnowflake user_id, void* callback_data, DiscordUserCallback callback) {
    struct DiscordUser user;
    memset(&user, 0, sizeof(user));
    user.id = user_id;
    strncpy(user.username, "StubUser", sizeof(user.username));
    if (callback) callback(callback_data, DiscordResult_Ok, &user);
}
static enum EDiscordResult stub_get_current_user_premium_type(struct IDiscordUserManager* manager, int32_t* premium_type) {
    *premium_type = 0;
    return DiscordResult_Ok;
}
static enum EDiscordResult stub_current_user_has_flag(struct IDiscordUserManager* manager, int32_t flag, bool* has_flag) {
    *has_flag = false;
    return DiscordResult_Ok;
}

static struct IDiscordUserManager s_stubUserManager = {
    stub_get_current_user,
    stub_get_user,
    stub_get_current_user_premium_type,
    stub_current_user_has_flag,
};

// Stub overlay manager
static void stub_overlay_is_enabled(struct IDiscordOverlayManager* manager, bool* enabled) {
    if (enabled) *enabled = false;
}
static void stub_overlay_is_locked(struct IDiscordOverlayManager* manager, bool* locked) {
    if (locked) *locked = true;
}
static void stub_overlay_set_locked(struct IDiscordOverlayManager* manager, bool locked, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_overlay_open_activity_invite(struct IDiscordOverlayManager* manager, enum EDiscordActivityActionType type, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_overlay_open_guild_invite(struct IDiscordOverlayManager* manager, const char* code, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}
static void stub_overlay_open_voice_settings(struct IDiscordOverlayManager* manager, void* callback_data, DiscordResultCallback callback) {
    if (callback) callback(callback_data, DiscordResult_Ok);
}

static struct IDiscordOverlayManager s_stubOverlayManager = {
    stub_overlay_is_enabled,
    stub_overlay_is_locked,
    stub_overlay_set_locked,
    stub_overlay_open_activity_invite,
    stub_overlay_open_guild_invite,
    stub_overlay_open_voice_settings,
};

// Stub core
static void stub_destroy(struct IDiscordCore* core) {
    // no-op for stub
}
static enum EDiscordResult stub_run_callbacks(struct IDiscordCore* core) {
    return DiscordResult_Ok;
}
static void stub_set_log_hook(struct IDiscordCore* core, int min_level, void* hook_data, void (*hook)(void* hook_data, int level, const char* message)) {
    // no-op
}
static struct IDiscordActivityManager* stub_get_activity_manager(struct IDiscordCore* core) {
    return &s_stubActivityManager;
}
static struct IDiscordUserManager* stub_get_user_manager(struct IDiscordCore* core) {
    return &s_stubUserManager;
}
static struct IDiscordOverlayManager* stub_get_overlay_manager(struct IDiscordCore* core) {
    return &s_stubOverlayManager;
}

static struct IDiscordCore s_stubCore = {
    stub_destroy,
    stub_run_callbacks,
    stub_set_log_hook,
    stub_get_activity_manager,
    stub_get_user_manager,
    stub_get_overlay_manager,
};

// Main entry point - stub implementation
enum EDiscordResult DiscordCreate(DiscordVersion version, struct DiscordCreateParams* params, struct IDiscordCore** result) {
    if (!result) return DiscordResult_InvalidPayload;

    printf("[Discord SDK STUB] DiscordCreate called - using stub implementation.\n");
    printf("[Discord SDK STUB] Replace with real discord_game_sdk.dll for actual Discord integration.\n");

    *result = &s_stubCore;
    return DiscordResult_Ok;
}
