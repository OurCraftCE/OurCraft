#pragma once

// Minecraft Mod API - C interface for ABI stability
// Mods are compiled as shared libraries (.dll on Windows, .so on Linux)
// and export a GetModInfo function that returns lifecycle callbacks.

#include "ModTypes.h"
#include "ModEvents.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Mod lifecycle info - every mod DLL must export GetModInfo()
// ============================================================

typedef struct ModInfo {
    // Mod metadata
    const char* modId;          // Unique mod identifier e.g. "mymod"
    const char* modName;        // Human-readable name
    const char* modVersion;     // Version string e.g. "1.0.0"
    const char* modAuthor;      // Author name

    // Lifecycle callbacks (all optional - set to NULL if unused)
    void (*onInitialize)(void);
    void (*onShutdown)(void);
    void (*onTick)(float deltaTime);
    void (*onWorldLoad)(ModWorld world);
    void (*onWorldUnload)(ModWorld world);
} ModInfo;

// Function type that every mod DLL must export
typedef ModInfo* (*GetModInfoFunc)(void);

// ============================================================
// Block / Item / Recipe / Entity Registration
// ============================================================

// Register a custom block. Returns the assigned block ID, or -1 on failure.
MODAPI int  ModAPI_RegisterBlock(const ModBlockDef* def);

// Register a custom item. Returns the assigned item ID, or -1 on failure.
MODAPI int  ModAPI_RegisterItem(const ModItemDef* def);

// Register a crafting recipe. Returns 0 on success, -1 on failure.
MODAPI int  ModAPI_RegisterRecipe(const ModRecipeDef* def);

// Register a custom entity type. Returns an entity type ID, or -1 on failure.
MODAPI int  ModAPI_RegisterEntityType(const ModEntityDef* def);

// ============================================================
// World Access
// ============================================================

// Get the block (tile) ID at the given position
MODAPI int  ModAPI_GetBlock(ModWorld world, int x, int y, int z);

// Set the block (tile) ID at the given position. Returns non-zero on success.
MODAPI int  ModAPI_SetBlock(ModWorld world, int x, int y, int z, int tileId);

// Get auxiliary/damage data for a block
MODAPI int  ModAPI_GetBlockData(ModWorld world, int x, int y, int z);

// Set auxiliary/damage data for a block. Returns non-zero on success.
MODAPI int  ModAPI_SetBlockData(ModWorld world, int x, int y, int z, int data);

// Set block and data in one call. Returns non-zero on success.
MODAPI int  ModAPI_SetBlockAndData(ModWorld world, int x, int y, int z, int tileId, int data);

// ============================================================
// Player API
// ============================================================

// Send a chat message to a player
MODAPI void ModAPI_SendChatMessage(ModPlayer player, const char* message);

// Get the player's current position
MODAPI ModVec3 ModAPI_GetPlayerPosition(ModPlayer player);

// Give an item to a player's inventory. Returns non-zero on success.
MODAPI int  ModAPI_GiveItem(ModPlayer player, int itemId, int count, int auxData);

// Get the player's name (returns pointer to internal buffer, do not free)
MODAPI const char* ModAPI_GetPlayerName(ModPlayer player);

// Get the player's health (half-hearts)
MODAPI int  ModAPI_GetPlayerHealth(ModPlayer player);

// ============================================================
// Event System
// ============================================================

// Register an event handler. Returns a handle for unregistration.
MODAPI int  ModAPI_RegisterEventHandler(ModEventType type, ModEventHandler handler, void* userData);

// Unregister a previously registered event handler.
MODAPI void ModAPI_UnregisterEventHandler(int handle);

// ============================================================
// Logging
// ============================================================

// Log an informational message (prefixed with [ModID])
MODAPI void ModAPI_Log(const char* modId, const char* message);

// Log a warning message
MODAPI void ModAPI_LogWarning(const char* modId, const char* message);

// Log an error message
MODAPI void ModAPI_LogError(const char* modId, const char* message);

// ============================================================
// Server API (no-ops on client)
// ============================================================

MODAPI int  ModAPI_GetPlayerList(ModPlayerInfo** outList, int* outCount);
MODAPI void ModAPI_FreePlayerList(ModPlayerInfo* list);
MODAPI void ModAPI_KickPlayer(ModPlayer player, const char* reason);
MODAPI void ModAPI_BanPlayer(const char* playerName, const char* reason);
MODAPI int  ModAPI_RegisterCommand(const char* commandName, const char* description,
                                    ModCommandHandler handler, void* userData);
MODAPI void ModAPI_UnregisterCommand(int handle);
MODAPI void ModAPI_BroadcastChat(const char* message);
MODAPI void ModAPI_TeleportPlayer(ModPlayer player, double x, double y, double z);
MODAPI void ModAPI_SetPlayerGamemode(ModPlayer player, int gamemode);

#ifdef __cplusplus
}
#endif
