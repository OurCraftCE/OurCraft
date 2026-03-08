#pragma once

#include "ModTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Event types fired by the game engine
typedef enum ModEventType {
    MOD_EVENT_NONE = 0,
    MOD_EVENT_BLOCK_BREAK,          // Player broke a block
    MOD_EVENT_BLOCK_PLACE,          // Player placed a block
    MOD_EVENT_PLAYER_JOIN,          // Player joined the world
    MOD_EVENT_PLAYER_LEAVE,         // Player left the world
    MOD_EVENT_PLAYER_CHAT,          // Player sent a chat message
    MOD_EVENT_PLAYER_DEATH,         // Player died
    MOD_EVENT_PLAYER_RESPAWN,       // Player respawned
    MOD_EVENT_ENTITY_SPAWN,         // Entity spawned
    MOD_EVENT_ENTITY_DEATH,         // Entity died
    MOD_EVENT_WORLD_LOAD,           // World finished loading
    MOD_EVENT_WORLD_UNLOAD,         // World unloading
    MOD_EVENT_WORLD_TICK,           // World tick (20Hz)
    MOD_EVENT_COUNT
} ModEventType;

// Block break/place event data
typedef struct ModBlockEvent {
    ModBlockPos pos;
    int         tileId;
    int         tileData;
    ModPlayer   player;
} ModBlockEvent;

// Player event data
typedef struct ModPlayerEvent {
    ModPlayer   player;
    const char* message;        // For chat events
} ModPlayerEvent;

// Entity event data
typedef struct ModEntityEvent {
    ModEntity   entity;
    ModVec3     position;
} ModEntityEvent;

// World event data
typedef struct ModWorldEvent {
    ModWorld    world;
} ModWorldEvent;

// Union of all event data
typedef struct ModEvent {
    ModEventType type;
    union {
        ModBlockEvent   block;
        ModPlayerEvent  playerEvt;
        ModEntityEvent  entityEvt;
        ModWorldEvent   worldEvt;
    } data;
} ModEvent;

// Event handler callback function type
// Return non-zero to cancel the event (if cancellable)
typedef int (*ModEventHandler)(const ModEvent* event, void* userData);

#ifdef __cplusplus
}
#endif
