#include "CraftingOverlay.h"
#include "RmlUIManager.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

// Bridge functions defined in OurCraft.cpp
extern "C" {
    int INV_GetItemId(int slot);
    int INV_GetItemCount(int slot);
    bool INV_HasPlayer();
    int CRAFT_GetSlotItemId(int slot);
    int CRAFT_GetSlotItemCount(int slot);
}

CraftingOverlay& CraftingOverlay::Get() {
    static CraftingOverlay instance;
    return instance;
}

void CraftingOverlay::Init() {
    if (m_doc) return;

    auto* context = RmlUIManager::Get().GetContext();
    if (!context) {
        SDL_Log("[CraftingOverlay] No RmlUi context available");
        return;
    }

    m_doc = context->LoadDocument("Common/UI/BedrockUI/crafting.rml");
    if (!m_doc) {
        SDL_Log("[CraftingOverlay] Failed to load crafting.rml");
        return;
    }

    memset(m_cachedItemIds, -1, sizeof(m_cachedItemIds));
    memset(m_cachedItemCounts, -1, sizeof(m_cachedItemCounts));
    memset(m_cachedCraftIds, -1, sizeof(m_cachedCraftIds));
    memset(m_cachedCraftCounts, -1, sizeof(m_cachedCraftCounts));
    m_dirty = true;

    SDL_Log("[CraftingOverlay] Initialized");
}

void CraftingOverlay::Shutdown() {
    if (m_doc) {
        m_doc->Close();
        m_doc = nullptr;
    }
    m_visible = false;
}

void CraftingOverlay::Show() {
    if (!m_doc) Init();
    if (m_doc) {
        m_doc->Show();
        m_visible = true;
        m_dirty = true;
    }
}

void CraftingOverlay::Hide() {
    if (m_doc) {
        m_doc->Hide();
        m_visible = false;
    }
}

void CraftingOverlay::Tick() {
    if (!m_doc || !m_visible) return;
    if (!INV_HasPlayer()) return;

    bool changed = false;

    for (int i = 0; i < 36; i++) {
        int id = INV_GetItemId(i);
        int count = INV_GetItemCount(i);
        if (id != m_cachedItemIds[i] || count != m_cachedItemCounts[i]) {
            m_cachedItemIds[i] = id;
            m_cachedItemCounts[i] = count;
            changed = true;
        }
    }

    for (int i = 0; i < 10; i++) {
        int id = CRAFT_GetSlotItemId(i);
        int count = CRAFT_GetSlotItemCount(i);
        if (id != m_cachedCraftIds[i] || count != m_cachedCraftCounts[i]) {
            m_cachedCraftIds[i] = id;
            m_cachedCraftCounts[i] = count;
            changed = true;
        }
    }

    if (changed || m_dirty) {
        RefreshAllSlots();
        m_dirty = false;
    }
}

void CraftingOverlay::UpdateSlot(const char* elementId, int itemId, int itemCount) {
    auto* el = m_doc->GetElementById(elementId);
    if (!el) return;

    if (itemId <= 0 || itemCount <= 0) {
        el->SetInnerRML("");
    } else {
        char buf[32];
        if (itemCount > 1)
            snprintf(buf, sizeof(buf), "%d:%d", itemId, itemCount);
        else
            snprintf(buf, sizeof(buf), "%d", itemId);
        el->SetInnerRML(buf);
    }
}

void CraftingOverlay::RefreshAllSlots() {
    char id[16];

    for (int i = 0; i < 36; i++) {
        snprintf(id, sizeof(id), "inv_%d", i);
        UpdateSlot(id, m_cachedItemIds[i], m_cachedItemCounts[i]);
    }

    for (int i = 0; i < 9; i++) {
        snprintf(id, sizeof(id), "craft_%d", i);
        UpdateSlot(id, m_cachedCraftIds[i], m_cachedCraftCounts[i]);
    }

    // crafting result (slot 9)
    UpdateSlot("craft_result", m_cachedCraftIds[9], m_cachedCraftCounts[9]);
}
