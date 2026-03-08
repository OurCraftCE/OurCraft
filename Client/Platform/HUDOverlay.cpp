#include "HUDOverlay.h"
#include "RmlUIManager.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdio>
#include <cmath>

// bridge functions defined in OurCraft.cpp
extern "C" {
    int HUD_GetPlayerHealth();
    int HUD_GetPlayerMaxHealth();
    int HUD_GetPlayerFood();
    int HUD_GetPlayerActiveSlot();
    float HUD_GetPlayerXPProgress();
    int HUD_GetPlayerXPLevel();
    int HUD_GetPlayerArmor();
    int HUD_GetPlayerAir();
    bool HUD_HasPlayer();
    bool HUD_IsInGame();
}

static const int MAX_CHAT_LINES = 10;
static const int DEFAULT_AIR_SUPPLY = 300; // Entity::TOTAL_AIR_SUPPLY = 20*15

HUDOverlay& HUDOverlay::Get() {
    static HUDOverlay instance;
    return instance;
}

void HUDOverlay::Init() {
    if (m_doc) return; // already initialized

    auto* context = RmlUIManager::Get().GetContext();
    if (!context) {
        SDL_Log("[HUDOverlay] No RmlUi context available");
        return;
    }

    m_doc = context->LoadDocument("Common/UI/BedrockUI/hud.rml");
    if (!m_doc) {
        SDL_Log("[HUDOverlay] Failed to load hud.rml");
        return;
    }

    // reset cached state so everything updates on first tick
    m_lastHealth = -1;
    m_lastMaxHealth = -1;
    m_lastFood = -1;
    m_lastSlot = -1;
    m_lastArmor = -1;
    m_lastAir = -1;
    m_lastXpProgress = -1.0f;
    m_lastXpLevel = -1;
    m_chatLineCount = 0;

    SDL_Log("[HUDOverlay] Initialized");
}

void HUDOverlay::Shutdown() {
    if (m_doc) {
        m_doc->Close();
        m_doc = nullptr;
    }
    m_visible = false;
}

void HUDOverlay::Show() {
    if (m_doc) {
        m_doc->Show();
        m_visible = true;
    }
}

void HUDOverlay::Hide() {
    if (m_doc) {
        m_doc->Hide();
        m_visible = false;
    }
}

void HUDOverlay::Tick() {
    if (!m_doc || !m_visible) return;
    if (!HUD_IsInGame() || !HUD_HasPlayer()) return;

    int health = HUD_GetPlayerHealth();
    int maxHealth = HUD_GetPlayerMaxHealth();
    int food = HUD_GetPlayerFood();
    int slot = HUD_GetPlayerActiveSlot();
    float xpProgress = HUD_GetPlayerXPProgress();
    int xpLevel = HUD_GetPlayerXPLevel();
    int armor = HUD_GetPlayerArmor();
    int air = HUD_GetPlayerAir();

    UpdateHealth(health, maxHealth);
    UpdateHunger(food);
    UpdateXP(xpProgress, xpLevel);
    UpdateActiveSlot(slot);
    UpdateArmor(armor);
    UpdateAir(air);
}

void HUDOverlay::UpdateHealth(int health, int maxHealth) {
    if (health == m_lastHealth && maxHealth == m_lastMaxHealth) return;
    m_lastHealth = health;
    m_lastMaxHealth = maxHealth;

    auto* el = m_doc->GetElementById("health_bar");
    if (!el) return;

    Rml::String html;
    int hearts = maxHealth / 2;
    for (int i = 0; i < hearts; i++) {
        bool full = (health >= (i + 1) * 2);
        bool half = (!full && health >= i * 2 + 1);
        const char* cls = full ? "heart" : (half ? "heart half" : "heart empty");
        html += "<div class=\"";
        html += cls;
        html += "\"></div>";
    }
    el->SetInnerRML(html);
}

void HUDOverlay::UpdateHunger(int food) {
    if (food == m_lastFood) return;
    m_lastFood = food;

    auto* el = m_doc->GetElementById("hunger_bar");
    if (!el) return;

    Rml::String html;
    for (int i = 0; i < 10; i++) {
        bool full = (food >= (i + 1) * 2);
        bool half = (!full && food >= i * 2 + 1);
        const char* cls = full ? "hunger" : (half ? "hunger half" : "hunger empty");
        html += "<div class=\"";
        html += cls;
        html += "\"></div>";
    }
    el->SetInnerRML(html);
}

void HUDOverlay::UpdateXP(float progress, int level) {
    bool progressChanged = (std::abs(progress - m_lastXpProgress) > 0.001f);
    bool levelChanged = (level != m_lastXpLevel);

    if (progressChanged) {
        m_lastXpProgress = progress;
        auto* bar = m_doc->GetElementById("xp_bar_fill");
        if (bar) {
                // width as percentage of parent (364dp)
            int widthDp = (int)(progress * 364.0f);
            char buf[64];
            snprintf(buf, sizeof(buf), "%ddp", widthDp);
            bar->SetProperty("width", buf);
        }
    }

    if (levelChanged) {
        m_lastXpLevel = level;
        auto* el = m_doc->GetElementById("xp_level");
        if (el) {
            if (level > 0) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", level);
                el->SetInnerRML(buf);
            } else {
                el->SetInnerRML("");
            }
        }
    }
}

void HUDOverlay::UpdateActiveSlot(int slot) {
    if (slot == m_lastSlot) return;

    if (m_lastSlot >= 0 && m_lastSlot < 9) {
        char id[16];
        snprintf(id, sizeof(id), "slot_%d", m_lastSlot);
        auto* oldEl = m_doc->GetElementById(id);
        if (oldEl) {
            oldEl->SetAttribute("class", "hotbar-slot");
        }
    }

    if (slot >= 0 && slot < 9) {
        char id[16];
        snprintf(id, sizeof(id), "slot_%d", slot);
        auto* newEl = m_doc->GetElementById(id);
        if (newEl) {
            newEl->SetAttribute("class", "hotbar-slot active");
        }
    }

    m_lastSlot = slot;
}

void HUDOverlay::UpdateArmor(int armor) {
    if (armor == m_lastArmor) return;
    m_lastArmor = armor;

    auto* el = m_doc->GetElementById("armor_bar");
    if (!el) return;

    if (armor <= 0) {
        el->SetInnerRML("");
        return;
    }

    Rml::String html;
    // armor value is 0-20, each icon = 2 points
    int icons = (armor + 1) / 2;
    for (int i = 0; i < icons; i++) {
        html += "<div class=\"armor-icon\"></div>";
    }
    el->SetInnerRML(html);
}

void HUDOverlay::UpdateAir(int air) {
    if (air == m_lastAir) return;
    m_lastAir = air;

    auto* el = m_doc->GetElementById("air_bar");
    if (!el) return;

    if (air >= DEFAULT_AIR_SUPPLY) {
        el->SetInnerRML("");
        return;
    }

    Rml::String html;
    // air is 0-300, each bubble = 30 ticks; round up
    int bubbles = (air + 29) / 30;
    if (bubbles < 0) bubbles = 0;
    if (bubbles > 10) bubbles = 10;
    for (int i = 0; i < bubbles; i++) {
        html += "<div class=\"bubble\"></div>";
    }
    el->SetInnerRML(html);
}

void HUDOverlay::AddChatMessage(const char* message) {
    if (!m_doc) return;

    auto* container = m_doc->GetElementById("chat_container");
    if (!container) return;

    if (m_chatLineCount >= MAX_CHAT_LINES) {
        auto* first = container->GetFirstChild();
        if (first) {
            container->RemoveChild(first);
            m_chatLineCount--;
        }
    }

    Rml::String currentHtml = container->GetInnerRML();
    currentHtml += "<div class=\"chat-line\">";
    currentHtml += message;
    currentHtml += "</div>";
    container->SetInnerRML(currentHtml);
    m_chatLineCount++;
}
