#pragma once
#include <RmlUi/Core.h>
#include <string>
#include <functional>

union SDL_Event;

enum class LauncherScreen {
    Login,
    Main,
    Download,
    Settings
};

class RmlBgfxRenderer;
class RmlSDLSystemInterface;

class LauncherUI {
public:
    bool Init(int width, int height);
    void Shutdown();
    void SetViewportSize(int width, int height);
    void ProcessEvent(const SDL_Event& event);
    void Update();
    void Render();

    void ShowScreen(LauncherScreen screen);
    LauncherScreen GetCurrentScreen() const { return m_CurrentScreen; }

    void SetUsername(const std::string& name);
    void SetDownloadProgress(float progress, const std::string& label);
    void SetPlayEnabled(bool enabled);
    void SetStatusText(const std::string& text);

    std::function<void()> OnLoginClicked;
    std::function<void()> OnPlayClicked;
    std::function<void()> OnSettingsClicked;
    std::function<void()> OnQuitClicked;
    std::function<void()> OnLogoutClicked;

private:
    Rml::Context* m_Context = nullptr;
    Rml::ElementDocument* m_Document = nullptr;
    LauncherScreen m_CurrentScreen = LauncherScreen::Login;

    RmlBgfxRenderer* m_Renderer = nullptr;
    RmlSDLSystemInterface* m_System = nullptr;

    class EventListener;
    EventListener* m_Listener = nullptr;

    void LoadScreen(const std::string& rmlFile);
};
