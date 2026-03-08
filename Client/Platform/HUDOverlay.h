#pragma once

#include <string>

namespace Rml { class ElementDocument; class Element; }

class HUDOverlay {
public:
    static HUDOverlay& Get();

    void Init();
    void Shutdown();
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void Tick();
    void AddChatMessage(const char* message);

private:
    HUDOverlay() = default;
    void UpdateHealth(int health, int maxHealth);
    void UpdateHunger(int food);
    void UpdateXP(float progress, int level);
    void UpdateActiveSlot(int slot);
    void UpdateArmor(int armor);
    void UpdateAir(int air);

    Rml::ElementDocument* m_doc = nullptr;
    bool m_visible = false;

    int m_lastHealth = -1;
    int m_lastMaxHealth = -1;
    int m_lastFood = -1;
    int m_lastSlot = -1;
    int m_lastArmor = -1;
    int m_lastAir = -1;
    float m_lastXpProgress = -1.0f;
    int m_lastXpLevel = -1;
    int m_chatLineCount = 0;
};
