#pragma once

#include <string>

namespace Rml { class ElementDocument; class Element; class EventListener; }

class ChatOverlay {
public:
    static ChatOverlay& Get();

    void Init();
    void Shutdown();
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void SendCurrentMessage();
    void AddMessage(const char* message);

private:
    ChatOverlay() = default;

    Rml::ElementDocument* m_doc = nullptr;
    Rml::EventListener* m_sendListener = nullptr;
    bool m_visible = false;
    int m_historyCount = 0;
};
