// ModAPI Backend Implementation
// This file is compiled as part of the game executable, so it has full
// access to game internals via proper includes.

#include "stdafx.h"
#include "ModAPIImpl.h"

// Game headers for Level, Player, Entity, etc.
#include "..\..\World\Level\Level.h"
#include "..\..\World\Player\Player.h"
#include "..\..\World\Entities\Entity.h"
#include "..\..\World\Math\Inventory.h"
#include "..\..\World\Items\ItemInstance.h"
#include "..\..\World\Tiles\Tile.h"
#include "..\..\World\Items\Item.h"

#include <cstdio>
#include <cstring>
#include <string>

// ============================================================
// ModAPIImpl singleton
// ============================================================

ModAPIImpl ModAPIImpl::s_instance;

ModAPIImpl& ModAPIImpl::Get()
{
    return s_instance;
}

ModAPIImpl::ModAPIImpl()
    : m_nextBlockId(256)       // Vanilla tiles use 0-255
    , m_nextItemId(2268)       // After vanilla items (safe starting point)
    , m_nextEntityTypeId(200)  // After vanilla entity types
    , m_nextEventHandle(1)
{
}

int ModAPIImpl::AllocateBlockId(const ModBlockDef* def)
{
    if (!def || !def->name) return -1;
    if (m_nextBlockId >= Tile::TILE_NUM_COUNT) {
        printf("[ModAPI] Error: Block ID limit (%d) reached\n", Tile::TILE_NUM_COUNT);
        return -1;
    }
    int id = m_nextBlockId++;
    printf("[ModAPI] Registered block '%s' with ID %d\n", def->name, id);
    return id;
}

int ModAPIImpl::AllocateItemId(const ModItemDef* def)
{
    if (!def || !def->name) return -1;
    if (m_nextItemId >= Item::ITEM_NUM_COUNT) {
        printf("[ModAPI] Error: Item ID limit (%d) reached\n", Item::ITEM_NUM_COUNT);
        return -1;
    }
    int id = m_nextItemId++;
    printf("[ModAPI] Registered item '%s' with ID %d\n", def->name, id);
    return id;
}

int ModAPIImpl::AllocateEntityTypeId(const ModEntityDef* def)
{
    if (!def || !def->name) return -1;
    int id = m_nextEntityTypeId++;
    printf("[ModAPI] Registered entity type '%s' with ID %d\n", def->name, id);
    return id;
}

int ModAPIImpl::AddEventHandler(ModEventType type, ModEventHandler handler, void* userData)
{
    EventHandlerEntry entry;
    entry.handle = m_nextEventHandle++;
    entry.type = type;
    entry.handler = handler;
    entry.userData = userData;
    m_eventHandlers.push_back(entry);
    return entry.handle;
}

void ModAPIImpl::RemoveEventHandler(int handle)
{
    for (size_t i = 0; i < m_eventHandlers.size(); i++) {
        if (m_eventHandlers[i].handle == handle) {
            m_eventHandlers.erase(m_eventHandlers.begin() + i);
            return;
        }
    }
}

int ModAPIImpl::FireEvent(const ModEvent* event)
{
    if (!event) return 0;
    int cancelled = 0;
    for (size_t i = 0; i < m_eventHandlers.size(); i++) {
        if (m_eventHandlers[i].type == event->type) {
            int result = m_eventHandlers[i].handler(event, m_eventHandlers[i].userData);
            if (result != 0) cancelled = 1;
        }
    }
    return cancelled;
}

void ModAPIImpl::SetCurrentModId(const char* modId)
{
    (void)modId; // Reserved for future use
}

// ============================================================
// Helper: wstring <-> narrow string conversion
// ============================================================

static std::wstring ToWide(const char* str)
{
    if (!str) return L"";
    std::wstring result;
    while (*str) {
        result += (wchar_t)(unsigned char)*str;
        str++;
    }
    return result;
}

// Thread-local buffer for returning narrow strings from wstring fields
static char s_nameBuffer[256];

static const char* ToNarrow(const std::wstring& ws)
{
    size_t len = ws.length();
    if (len >= sizeof(s_nameBuffer)) len = sizeof(s_nameBuffer) - 1;
    for (size_t i = 0; i < len; i++) {
        s_nameBuffer[i] = (char)ws[i];
    }
    s_nameBuffer[len] = '\0';
    return s_nameBuffer;
}

// ============================================================
// C API Implementation - Block/Item Registration
// ============================================================

extern "C" {

MODAPI int ModAPI_RegisterBlock(const ModBlockDef* def)
{
    return ModAPIImpl::Get().AllocateBlockId(def);
}

MODAPI int ModAPI_RegisterItem(const ModItemDef* def)
{
    return ModAPIImpl::Get().AllocateItemId(def);
}

MODAPI int ModAPI_RegisterRecipe(const ModRecipeDef* def)
{
    if (!def) return -1;
    printf("[ModAPI] Registered recipe producing item %d x%d\n", def->resultId, def->resultCount);
    return 0;
}

MODAPI int ModAPI_RegisterEntityType(const ModEntityDef* def)
{
    return ModAPIImpl::Get().AllocateEntityTypeId(def);
}

// ============================================================
// C API Implementation - World Access
// ============================================================

MODAPI int ModAPI_GetBlock(ModWorld world, int x, int y, int z)
{
    if (!world) return 0;
    Level* level = reinterpret_cast<Level*>(world);
    return level->getTile(x, y, z);
}

MODAPI int ModAPI_SetBlock(ModWorld world, int x, int y, int z, int tileId)
{
    if (!world) return 0;
    Level* level = reinterpret_cast<Level*>(world);
    return level->setTile(x, y, z, tileId) ? 1 : 0;
}

MODAPI int ModAPI_GetBlockData(ModWorld world, int x, int y, int z)
{
    if (!world) return 0;
    Level* level = reinterpret_cast<Level*>(world);
    return level->getData(x, y, z);
}

MODAPI int ModAPI_SetBlockData(ModWorld world, int x, int y, int z, int data)
{
    if (!world) return 0;
    Level* level = reinterpret_cast<Level*>(world);
    level->setData(x, y, z, data);
    return 1;
}

MODAPI int ModAPI_SetBlockAndData(ModWorld world, int x, int y, int z, int tileId, int data)
{
    if (!world) return 0;
    Level* level = reinterpret_cast<Level*>(world);
    return level->setTileAndData(x, y, z, tileId, data) ? 1 : 0;
}

// ============================================================
// C API Implementation - Player API
// ============================================================

MODAPI void ModAPI_SendChatMessage(ModPlayer player, const char* message)
{
    if (!player || !message) return;
    Player* p = reinterpret_cast<Player*>(player);
    std::wstring wmsg = ToWide(message);
    p->sendMessage(wmsg);
}

MODAPI ModVec3 ModAPI_GetPlayerPosition(ModPlayer player)
{
    ModVec3 pos = {0.0, 0.0, 0.0};
    if (!player) return pos;
    Player* p = reinterpret_cast<Player*>(player);
    // Entity base class has x, y, z as public doubles
    pos.x = p->x;
    pos.y = p->y;
    pos.z = p->z;
    return pos;
}

MODAPI int ModAPI_GiveItem(ModPlayer player, int itemId, int count, int auxData)
{
    if (!player || count <= 0) return 0;
    Player* p = reinterpret_cast<Player*>(player);
    auto item = std::make_shared<ItemInstance>(itemId, count, auxData);
    return p->inventory->add(item) ? 1 : 0;
}

MODAPI const char* ModAPI_GetPlayerName(ModPlayer player)
{
    if (!player) return "";
    Player* p = reinterpret_cast<Player*>(player);
    return ToNarrow(p->name);
}

MODAPI int ModAPI_GetPlayerHealth(ModPlayer player)
{
    if (!player) return 0;
    Player* p = reinterpret_cast<Player*>(player);
    return p->getHealth();
}

// ============================================================
// C API Implementation - Event System
// ============================================================

MODAPI int ModAPI_RegisterEventHandler(ModEventType type, ModEventHandler handler, void* userData)
{
    return ModAPIImpl::Get().AddEventHandler(type, handler, userData);
}

MODAPI void ModAPI_UnregisterEventHandler(int handle)
{
    ModAPIImpl::Get().RemoveEventHandler(handle);
}

// ============================================================
// C API Implementation - Logging
// ============================================================

MODAPI void ModAPI_Log(const char* modId, const char* message)
{
    printf("[%s] %s\n", modId ? modId : "Mod", message ? message : "");
}

MODAPI void ModAPI_LogWarning(const char* modId, const char* message)
{
    printf("[%s] WARNING: %s\n", modId ? modId : "Mod", message ? message : "");
}

MODAPI void ModAPI_LogError(const char* modId, const char* message)
{
    printf("[%s] ERROR: %s\n", modId ? modId : "Mod", message ? message : "");
}

} // extern "C"
