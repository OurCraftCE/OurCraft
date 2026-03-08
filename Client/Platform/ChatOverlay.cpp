#include "ChatOverlay.h"
#include "RmlUIManager.h"
#include "HUDOverlay.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

// Bridge function defined in OurCraft.cpp
extern "C" {
    void CHAT_SendMessage(const char* message);
}

static const int MAX_CHAT_HISTORY = 50;

class ChatSendListener : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override {
        ChatOverlay::Get().SendCurrentMessage();
    }
};

ChatOverlay& ChatOverlay::Get() {
    static ChatOverlay instance;
    return instance;
}

void ChatOverlay::Init() {
    if (m_doc) return;

    auto* context = RmlUIManager::Get().GetContext();
    if (!context) {
        SDL_Log("[ChatOverlay] No RmlUi context available");
        return;
    }

    m_doc = context->LoadDocument("Common/UI/BedrockUI/chat_input.rml");
    if (!m_doc) {
        SDL_Log("[ChatOverlay] Failed to load chat_input.rml");
        return;
    }

    if (!m_sendListener) {
        m_sendListener = new ChatSendListener();
    }
    if (auto* btn = m_doc->GetElementById("btn_send")) {
        btn->AddEventListener(Rml::EventId::Click, m_sendListener);
    }

    m_historyCount = 0;

    SDL_Log("[ChatOverlay] Initialized");
}

void ChatOverlay::Shutdown() {
    if (m_doc) {
        m_doc->Close();
        m_doc = nullptr;
    }
    delete m_sendListener;
    m_sendListener = nullptr;
    m_visible = false;
}

void ChatOverlay::Show() {
    if (!m_doc) Init();
    if (m_doc) {
        m_doc->Show();
        m_visible = true;

        if (auto* input = m_doc->GetElementById("chat_text")) {
            input->Focus();
            input->SetAttribute("value", "");
        }
    }
}

void ChatOverlay::Hide() {
    if (m_doc) {
        m_doc->Hide();
        m_visible = false;
    }
}

void ChatOverlay::SendCurrentMessage() {
    if (!m_doc) return;

    auto* input = m_doc->GetElementById("chat_text");
    if (!input) return;

    Rml::String value = input->GetAttribute<Rml::String>("value", "");
    if (value.empty()) return;

    CHAT_SendMessage(value.c_str());
    HUDOverlay::Get().AddChatMessage(value.c_str());
    AddMessage(value.c_str());
    input->SetAttribute("value", "");
}

void ChatOverlay::AddMessage(const char* message) {
    if (!m_doc) return;

    auto* container = m_doc->GetElementById("chat_history");
    if (!container) return;

    if (m_historyCount >= MAX_CHAT_HISTORY) {
        auto* first = container->GetFirstChild();
        if (first) {
            container->RemoveChild(first);
            m_historyCount--;
        }
    }

    Rml::String currentHtml = container->GetInnerRML();
    currentHtml += "<div class=\"chat-line\">";
    currentHtml += message;
    currentHtml += "</div>";
    container->SetInnerRML(currentHtml);
    m_historyCount++;
}
