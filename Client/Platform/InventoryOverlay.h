#pragma once

#include <string>

namespace Rml { class ElementDocument; class Element; }

class InventoryOverlay {
public:
    static InventoryOverlay& Get();

    void Init();
    void Shutdown();
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void Tick();

private:
    InventoryOverlay() = default;
    void UpdateSlot(const char* elementId, int itemId, int itemCount);
    void RefreshAllSlots();

    Rml::ElementDocument* m_doc = nullptr;
    bool m_visible = false;
    bool m_dirty = true;

    int m_cachedItemIds[40] = {};   // 0-35 inventory, 36-39 armor
    int m_cachedItemCounts[40] = {};
    int m_selectedSlot = -1;
};
