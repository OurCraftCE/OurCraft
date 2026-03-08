#include "ContainerOverlay.h"
#include "RmlUIManager.h"
#include "UIScreenManager.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <cstdio>

// Bridge functions implemented in OurCraft.cpp
extern "C" {
    void   CONTAINER_SlotClicked(int slot, int button, bool shift);
    int    CONTAINER_GetItemId(int slot);
    int    CONTAINER_GetItemCount(int slot);
    int    CONTAINER_GetSlotCount();
    const char* CONTAINER_GetItemName(int itemId);
}

void ContainerOverlay::Show(const char* rmlFile, const char* title) {
    auto* ctx = RmlUIManager::Get().GetContext();
    if (!ctx) return;

    std::string path = UIScreenManager::Get().ResolveUIPath(rmlFile);
    m_doc = ctx->LoadDocument(path.c_str());
    if (!m_doc) {
        SDL_Log("[ContainerOverlay] Failed to load %s", path.c_str());
        return;
    }
    if (title) {
        if (auto* el = m_doc->GetElementById("container_title"))
            el->SetInnerRML(title);
    }
    m_doc->Show();
    m_visible = true;
    RefreshSlots();
}

void ContainerOverlay::Hide() {
    if (m_doc) { m_doc->Close(); m_doc = nullptr; }
    m_visible = false;
}

void ContainerOverlay::Tick() {
    if (!m_visible || !m_doc) return;
    RefreshSlots();
    OnTick();
}

void ContainerOverlay::RefreshSlots() {
    if (!m_doc) return;
    int count = CONTAINER_GetSlotCount();
    for (int i = 0; i < count; i++) {
        char id[32]; snprintf(id, sizeof(id), "slot_%d", i);
        auto* el = m_doc->GetElementById(id);
        if (!el) continue;
        int itemId = CONTAINER_GetItemId(i);
        int cnt    = CONTAINER_GetItemCount(i);
        if (itemId <= 0) {
            el->SetInnerRML("");
            el->SetAttribute("class", "slot");
        } else {
            const char* name = CONTAINER_GetItemName(itemId);
            char buf[64];
            if (cnt > 1) snprintf(buf, sizeof(buf), "%s x%d", name ? name : "?", cnt);
            else         snprintf(buf, sizeof(buf), "%s", name ? name : "?");
            el->SetInnerRML(buf);
            el->SetAttribute("class", "slot has-item");
        }
    }
}

void ContainerOverlay::OnSlotClicked(int slotIndex, int mouseButton, bool shiftHeld) {
    CONTAINER_SlotClicked(slotIndex, mouseButton, shiftHeld);
    RefreshSlots();
}
