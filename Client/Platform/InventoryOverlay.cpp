#include "InventoryOverlay.h"
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
}

InventoryOverlay& InventoryOverlay::Get() {
    static InventoryOverlay instance;
    return instance;
}

void InventoryOverlay::Init() {
    if (m_doc) return;

    auto* context = RmlUIManager::Get().GetContext();
    if (!context) {
        SDL_Log("[InventoryOverlay] No RmlUi context available");
        return;
    }

    m_doc = context->LoadDocument("Common/UI/BedrockUI/inventory.rml");
    if (!m_doc) {
        SDL_Log("[InventoryOverlay] Failed to load inventory.rml");
        return;
    }

    memset(m_cachedItemIds, -1, sizeof(m_cachedItemIds));
    memset(m_cachedItemCounts, -1, sizeof(m_cachedItemCounts));
    m_dirty = true;
    m_selectedSlot = -1;

    SDL_Log("[InventoryOverlay] Initialized");
}

void InventoryOverlay::Shutdown() {
    if (m_doc) {
        m_doc->Close();
        m_doc = nullptr;
    }
    m_visible = false;
}

void InventoryOverlay::Show() {
    if (!m_doc) Init();
    if (m_doc) {
        m_doc->Show();
        m_visible = true;
        m_dirty = true;
    }
}

void InventoryOverlay::Hide() {
    if (m_doc) {
        m_doc->Hide();
        m_visible = false;
    }
}

void InventoryOverlay::Tick() {
    if (!m_doc || !m_visible) return;
    if (!INV_HasPlayer()) return;

    bool changed = false;
    for (int i = 0; i < 40; i++) {
        int id = INV_GetItemId(i);
        int count = INV_GetItemCount(i);
        if (id != m_cachedItemIds[i] || count != m_cachedItemCounts[i]) {
            m_cachedItemIds[i] = id;
            m_cachedItemCounts[i] = count;
            changed = true;
        }
    }

    if (changed || m_dirty) {
        RefreshAllSlots();
        m_dirty = false;
    }
}

void InventoryOverlay::UpdateSlot(const char* elementId, int itemId, int itemCount) {
    auto* el = m_doc->GetElementById(elementId);
    if (!el) return;

    if (itemId <= 0 || itemCount <= 0) {
        el->SetInnerRML("");
        el->SetAttribute("class", "slot");
    } else {
        char buf[32];
        if (itemCount > 1)
            snprintf(buf, sizeof(buf), "%d:%d", itemId, itemCount);
        else
            snprintf(buf, sizeof(buf), "%d", itemId);
        el->SetInnerRML(buf);
        el->SetAttribute("class", "slot has-item");
    }
}

void InventoryOverlay::RefreshAllSlots() {
    char id[16];

    for (int i = 0; i < 36; i++) {
        snprintf(id, sizeof(id), "inv_%d", i);
        UpdateSlot(id, m_cachedItemIds[i], m_cachedItemCounts[i]);
    }

    // armor slots 36-39
    for (int i = 0; i < 4; i++) {
        snprintf(id, sizeof(id), "armor_%d", i);
        UpdateSlot(id, m_cachedItemIds[36 + i], m_cachedItemCounts[36 + i]);
    }
}
