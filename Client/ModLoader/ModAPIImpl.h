#pragma once

#include "..\..\ModAPI\ModAPI.h"
#include <vector>
#include <map>

// Internal event handler entry
struct EventHandlerEntry {
    int             handle;
    ModEventType    type;
    ModEventHandler handler;
    void*           userData;
};

// ModAPI implementation state
class ModAPIImpl {
public:
    static ModAPIImpl& Get();

    // Block/Item ID allocation
    int  AllocateBlockId(const ModBlockDef* def);
    int  AllocateItemId(const ModItemDef* def);
    int  AllocateEntityTypeId(const ModEntityDef* def);

    // Event system
    int  AddEventHandler(ModEventType type, ModEventHandler handler, void* userData);
    void RemoveEventHandler(int handle);
    int  FireEvent(const ModEvent* event);

    // Set current mod context (for logging)
    void SetCurrentModId(const char* modId);

private:
    ModAPIImpl();

    int m_nextBlockId;      // Start allocating at 256
    int m_nextItemId;       // Start after vanilla items
    int m_nextEntityTypeId; // Custom entity type counter
    int m_nextEventHandle;  // Auto-incrementing event handle

    std::vector<EventHandlerEntry> m_eventHandlers;

    static ModAPIImpl s_instance;
};
