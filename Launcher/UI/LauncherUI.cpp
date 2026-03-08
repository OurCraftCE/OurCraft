#include "LauncherUI.h"
#include "../Platform/RmlBgfxRenderer.h"
#include "../Platform/RmlSystemInterface.h"
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <SDL3/SDL.h>

class LauncherUI::EventListener : public Rml::EventListener {
public:
    LauncherUI* ui;
    EventListener(LauncherUI* u) : ui(u) {}

    void ProcessEvent(Rml::Event& event) override {
        if (event.GetType() != "click") return;
        Rml::Element* el = event.GetCurrentElement();
        std::string id = el->GetId();

        if (id == "btn_login" && ui->OnLoginClicked) ui->OnLoginClicked();
        else if (id == "btn_play" && ui->OnPlayClicked) ui->OnPlayClicked();
        else if (id == "btn_settings" && ui->OnSettingsClicked) ui->OnSettingsClicked();
        else if (id == "btn_quit" && ui->OnQuitClicked) ui->OnQuitClicked();
        else if (id == "btn_back") ui->ShowScreen(LauncherScreen::Main);
        else if (id == "btn_logout" && ui->OnLogoutClicked) ui->OnLogoutClicked();
    }
};

bool LauncherUI::Init(int width, int height) {
    m_Renderer = new RmlBgfxRenderer();
    m_System = new RmlSDLSystemInterface();
    m_Listener = new EventListener(this);

    Rml::SetRenderInterface(m_Renderer);
    Rml::SetSystemInterface(m_System);
    Rml::Initialise();

    m_Renderer->Init(width, height);
    m_Renderer->SetViewId(200);

    // Load fonts -- try Mojangles first, fall back to system font
    if (!Rml::LoadFontFace("UI/rml/Mojangles.ttf", true)) {
        Rml::LoadFontFace("C:/Windows/Fonts/segoeui.ttf", true);
    }

    m_Context = Rml::CreateContext("launcher", Rml::Vector2i(width, height));
    if (!m_Context) return false;

    Rml::Debugger::Initialise(m_Context);
    return true;
}

void LauncherUI::Shutdown() {
    if (m_Document) { m_Document->Close(); m_Document = nullptr; }
    if (m_Context) { Rml::RemoveContext("launcher"); m_Context = nullptr; }
    Rml::Shutdown();
    if (m_Renderer) { m_Renderer->Shutdown(); delete m_Renderer; m_Renderer = nullptr; }
    delete m_System; m_System = nullptr;
    delete m_Listener; m_Listener = nullptr;
}

void LauncherUI::SetViewportSize(int width, int height) {
    if (m_Context) m_Context->SetDimensions(Rml::Vector2i(width, height));
    if (m_Renderer) m_Renderer->SetViewportSize(width, height);
}

void LauncherUI::ProcessEvent(const SDL_Event& event) {
    if (!m_Context) return;
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
        m_Context->ProcessMouseMove((int)event.motion.x, (int)event.motion.y, 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        m_Context->ProcessMouseButtonDown(event.button.button - 1, 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        m_Context->ProcessMouseButtonUp(event.button.button - 1, 0);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        m_Context->ProcessMouseWheel(Rml::Vector2f(event.wheel.x, event.wheel.y), 0);
        break;
    case SDL_EVENT_KEY_DOWN:
        m_Context->ProcessKeyDown((Rml::Input::KeyIdentifier)event.key.scancode, 0);
        break;
    case SDL_EVENT_KEY_UP:
        m_Context->ProcessKeyUp((Rml::Input::KeyIdentifier)event.key.scancode, 0);
        break;
    case SDL_EVENT_TEXT_INPUT:
        m_Context->ProcessTextInput(Rml::String(event.text.text));
        break;
    }
}

void LauncherUI::Update() {
    if (m_Context) m_Context->Update();
}

void LauncherUI::Render() {
    if (m_Renderer) m_Renderer->BeginFrame();
    if (m_Context) m_Context->Render();
}

void LauncherUI::ShowScreen(LauncherScreen screen) {
    m_CurrentScreen = screen;
    switch (screen) {
    case LauncherScreen::Login:    LoadScreen("UI/rml/login.rml"); break;
    case LauncherScreen::Main:     LoadScreen("UI/rml/main_screen.rml"); break;
    case LauncherScreen::Download: LoadScreen("UI/rml/download.rml"); break;
    case LauncherScreen::Settings: LoadScreen("UI/rml/settings.rml"); break;
    }
}

void LauncherUI::LoadScreen(const std::string& rmlFile) {
    if (m_Document) { m_Document->Close(); m_Document = nullptr; }
    m_Document = m_Context->LoadDocument(rmlFile);
    if (m_Document) {
        m_Document->Show();
        Rml::ElementList buttons;
        m_Document->GetElementsByTagName(buttons, "button");
        for (auto* btn : buttons) {
            btn->AddEventListener("click", m_Listener);
        }
    }
}

void LauncherUI::SetUsername(const std::string& name) {
    if (!m_Document) return;
    if (auto* el = m_Document->GetElementById("welcome_text"))
        el->SetInnerRML("Welcome, " + name);
}

void LauncherUI::SetDownloadProgress(float progress, const std::string& label) {
    if (!m_Document) return;
    if (auto* el = m_Document->GetElementById("download_label"))
        el->SetInnerRML(label);
    if (auto* el = m_Document->GetElementById("progress_fill")) {
        // Progress bar bg is 400dp wide, so fill = progress * 400dp
        int widthDp = (int)(progress * 400.0f);
        el->SetProperty("width", std::to_string(widthDp) + "dp");
    }
    if (auto* el = m_Document->GetElementById("progress_pct"))
        el->SetInnerRML(std::to_string((int)(progress * 100)) + "%");
}

void LauncherUI::SetPlayEnabled(bool enabled) {
    if (!m_Document) return;
    if (auto* el = m_Document->GetElementById("btn_play")) {
        if (enabled) el->RemoveAttribute("disabled");
        else el->SetAttribute("disabled", "");
    }
}

void LauncherUI::SetStatusText(const std::string& text) {
    if (!m_Document) return;
    if (auto* el = m_Document->GetElementById("status_text"))
        el->SetInnerRML(text);
}
