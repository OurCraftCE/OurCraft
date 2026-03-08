#pragma once

#ifdef _WIN32
    #define MODAPI __declspec(dllexport)
#else
    #define MODAPI __attribute__((visibility("default")))
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handles - mods never see the internal game types
typedef struct ModWorld_t* ModWorld;
typedef struct ModPlayer_t* ModPlayer;
typedef struct ModEntity_t* ModEntity;

// Position vector used throughout the API
typedef struct ModVec3 {
    double x, y, z;
} ModVec3;

// Block position (integer coordinates)
typedef struct ModBlockPos {
    int x, y, z;
} ModBlockPos;

// Block definition for custom block registration
typedef struct ModBlockDef {
    const char* name;           // Internal name e.g. "mymod:ruby_ore"
    const char* displayName;    // Display name e.g. "Ruby Ore"
    int         textureIndex;   // Texture index in terrain atlas
    int         renderShape;    // Render shape (0 = full block)
    float       hardness;       // Destroy time (stone = 1.5)
    float       resistance;     // Explosion resistance
    int         lightEmission;  // Light level 0-15
    int         lightBlock;     // Light blocking 0-15
    int         materialType;   // 0=air, 1=stone, 2=wood, 3=dirt, etc.
    int         isSolid;        // Non-zero if block is solid
} ModBlockDef;

// Item definition for custom item registration
typedef struct ModItemDef {
    const char* name;           // Internal name e.g. "mymod:ruby"
    const char* displayName;    // Display name
    int         textureIndex;   // Texture index in items atlas
    int         maxStackSize;   // Max stack size (default 64)
    int         maxDamage;      // Max durability (0 = no durability)
    int         isFood;         // Non-zero if food item
    int         foodValue;      // Half-drumsticks of food restored
    float       saturation;     // Saturation modifier
} ModItemDef;

// Recipe definition for crafting registration
typedef struct ModRecipeDef {
    int         resultId;       // Result item/block ID
    int         resultCount;    // How many produced
    int         resultData;     // Aux/damage data for result
    // Shaped recipe: 3x3 grid of item IDs (-1 = empty)
    int         grid[9];
    int         gridData[9];    // Aux data for each grid slot
    int         width, height;  // Shaped dimensions (0 = shapeless)
    // Shapeless: ingredients list (terminated by -1)
    int         ingredients[9];
    int         ingredientData[9];
    int         isShaped;       // Non-zero for shaped recipe
} ModRecipeDef;

// Entity definition for custom entity registration
typedef struct ModEntityDef {
    const char* name;           // Internal name
    const char* displayName;    // Display name
    float       width;          // Bounding box width
    float       height;         // Bounding box height
    int         maxHealth;      // Max health (half-hearts)
    int         isHostile;      // Non-zero if hostile mob
    int         fireImmune;     // Non-zero if immune to fire
} ModEntityDef;

// Player info for server-side mod queries
typedef struct ModPlayerInfo {
    ModPlayer handle;
    const char* name;
    double x, y, z;
    int dimension;
    float health;
    int gamemode;
} ModPlayerInfo;

// Custom command callback: (senderPlayer, argc, argv, userData) -> 0 on success
typedef int (*ModCommandHandler)(ModPlayer sender, int argc, const char** argv, void* userData);

#ifdef __cplusplus
}
#endif
