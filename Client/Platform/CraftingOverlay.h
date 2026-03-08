#pragma once

#include <string>

namespace Rml { class ElementDocument; class Element; }

class CraftingOverlay {
public:
    static CraftingOverlay& Get();

    void Init();
    void Shutdown();
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void Tick();

private:
    CraftingOverlay() = default;
    void UpdateSlot(const char* elementId, int itemId, int itemCount);
    void RefreshAllSlots();

    Rml::ElementDocument* m_doc = nullptr;
    bool m_visible = false;
    bool m_dirty = true;

    int m_cachedItemIds[36] = {};
    int m_cachedItemCounts[36] = {};

    // slots 0-8 grid, 9 result
    int m_cachedCraftIds[10] = {};
    int m_cachedCraftCounts[10] = {};
};
