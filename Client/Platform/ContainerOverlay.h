#pragma once
#include <string>

namespace Rml { class ElementDocument; }

class ContainerOverlay {
public:
    virtual ~ContainerOverlay() = default;

    void Show(const char* rmlFile, const char* title = nullptr);
    void Hide();
    void Tick();
    bool IsVisible() const { return m_visible; }
    void OnSlotClicked(int slotIndex, int mouseButton, bool shiftHeld);

protected:
    virtual void OnTick() {}

    Rml::ElementDocument* m_doc = nullptr;
    bool m_visible = false;

    void RefreshSlots();
};
