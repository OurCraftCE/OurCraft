#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <curl/curl.h>

#include "UI/LauncherUI.h"
#include "Auth/MicrosoftAuth.h"
#include "Auth/TokenStorage.h"
#include "Assets/AssetManager.h"
#include "Client/ClientManager.h"

#include <nlohmann/json.hpp>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif

namespace fs = std::filesystem;

static SDL_Window* g_Window = nullptr;
static int g_Width = 1280;
static int g_Height = 720;
static bool g_Running = true;

static LauncherUI g_UI;
static MicrosoftAuth g_Auth;
static AuthTokens g_Tokens;
static AssetManager g_AssetManager;
static ClientManager g_ClientManager;

static std::string g_InstallDir;
static std::string g_AuthFile;
static bool g_DevMode = false;
static std::atomic<bool> g_AuthInProgress{false};
static std::atomic<bool> g_AuthSuccess{false};
static std::atomic<bool> g_AuthFailed{false};

// Thread-safe message queue for UI status updates from background threads
static std::mutex g_StatusMutex;
static std::queue<std::string> g_StatusQueue;

// Thread-safe copy of tokens (written by auth thread, read by main thread)
static std::mutex g_TokenMutex;
static AuthTokens g_PendingTokens;

static void PushStatus(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_StatusMutex);
    g_StatusQueue.push(msg);
}

static bool PopStatus(std::string& msg) {
    std::lock_guard<std::mutex> lock(g_StatusMutex);
    if (g_StatusQueue.empty()) return false;
    msg = g_StatusQueue.front();
    g_StatusQueue.pop();
    return true;
}

static std::string GetAppDataDir() {
    char* appdata = nullptr;
    size_t len = 0;
    _dupenv_s(&appdata, &len, "APPDATA");
    std::string dir = std::string(appdata ? appdata : ".") + "/OurCraft";
    free(appdata);
    fs::create_directories(dir);
    return dir;
}

static bool InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    g_Window = SDL_CreateWindow("OurCraft Launcher", g_Width, g_Height, SDL_WINDOW_RESIZABLE);
    return g_Window != nullptr;
}

static bool InitBgfx() {
    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.resolution.width = g_Width;
    init.resolution.height = g_Height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    SDL_PropertiesID props = SDL_GetWindowProperties(g_Window);
    init.platformData.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    if (!bgfx::init(init)) return false;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a1a2eff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, g_Width, g_Height);
    return true;
}

// Run auth in background thread
static void AuthThread() {
    g_AuthInProgress = true;
    g_AuthSuccess = false;
    g_AuthFailed = false;

    AuthTokens tokens;
    bool success = g_Auth.Authenticate(tokens, [](const std::string& status) {
        SDL_Log("Auth: %s", status.c_str());
        PushStatus(status);
    });

    if (success) {
        TokenStorage::Save(g_AuthFile, tokens);
        {
            std::lock_guard<std::mutex> lock(g_TokenMutex);
            g_PendingTokens = tokens;
        }
        g_AuthSuccess = true;
    } else {
        PushStatus("Authentication failed. Please try again.");
        g_AuthFailed = true;
    }
    g_AuthInProgress = false;
}

static void OnLoginClicked() {
    if (g_AuthInProgress) return;
    g_UI.SetStatusText("Opening browser...");
    std::thread(AuthThread).detach();
}

// Run asset sync in background thread after auth
static std::atomic<bool> g_AssetsReady{false};
static std::atomic<bool> g_AssetSyncRunning{false};

static void AssetSyncThread() {
    g_AssetSyncRunning = true;
    AssetManager::Config acfg;
    acfg.installDir = g_InstallDir;
    {
        std::lock_guard<std::mutex> lock(g_TokenMutex);
        acfg.minecraftToken = g_Tokens.minecraftToken;
    }

    if (g_AssetManager.AssetsUpToDate(g_InstallDir)) {
        PushStatus("Assets up to date");
        g_AssetsReady = true;
    } else {
        g_AssetManager.SyncAssets(acfg, [](const std::string& label, float progress) {
            PushStatus(label);
        });
        g_AssetsReady = true;
    }
    g_AssetSyncRunning = false;
}

static void OnPlayClicked() {
    ClientManager::Config cfg;
    cfg.installDir = g_InstallDir;
    cfg.devMode = g_DevMode;

    if (g_ClientManager.Launch(cfg, g_InstallDir + "/assets", g_Tokens.minecraftToken, g_Tokens.username)) {
        g_Running = false;
    }
}

static void OnLogoutClicked() {
    TokenStorage::Delete(g_AuthFile);
    g_Tokens = AuthTokens{};
    g_UI.ShowScreen(LauncherScreen::Login);
}

int main(int argc, char* argv[]) {
    // Parse args
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--dev") g_DevMode = true;
    }

    // Auto-detect dev mode
    if (fs::exists("MinecraftConsoles.sln") || fs::exists("../MinecraftConsoles.sln"))
        g_DevMode = true;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_InstallDir = GetAppDataDir();
    g_AuthFile = g_InstallDir + "/auth.json";

    // Load settings (client_id override, etc.)
    {
        std::string settingsPath = g_InstallDir + "/settings.json";
        std::ifstream sf(settingsPath);
        if (sf) {
            try {
                auto settings = nlohmann::json::parse(sf);
                if (settings.contains("client_id") && settings["client_id"].is_string()) {
                    g_Auth.SetClientId(settings["client_id"].get<std::string>());
                    SDL_Log("Using custom client_id from settings.json");
                }
                if (settings.contains("dev_mode") && settings["dev_mode"].is_boolean()) {
                    g_DevMode = settings["dev_mode"].get<bool>();
                }
                if (settings.contains("releases_url") && settings["releases_url"].is_string()) {
                    g_ClientManager.SetReleasesUrl(settings["releases_url"].get<std::string>());
                    SDL_Log("Using custom releases_url from settings.json");
                }
            } catch (...) {}
        }
    }

    if (!InitSDL()) return 1;
    if (!InitBgfx()) return 1;
    if (!g_UI.Init(g_Width, g_Height)) return 1;

    // Set up callbacks
    g_UI.OnLoginClicked = OnLoginClicked;
    g_UI.OnPlayClicked = OnPlayClicked;
    g_UI.OnSettingsClicked = []() { g_UI.ShowScreen(LauncherScreen::Settings); };
    g_UI.OnLogoutClicked = OnLogoutClicked;
    g_UI.OnQuitClicked = []() { g_Running = false; };

    // Try auto-login with cached tokens
    bool autoLoggedIn = false;
    if (TokenStorage::Load(g_AuthFile, g_Tokens) && g_Auth.IsValid(g_Tokens)) {
        if (g_Auth.Refresh(g_Tokens, nullptr)) {
            TokenStorage::Save(g_AuthFile, g_Tokens);
            // Check if assets need syncing
            if (g_AssetManager.AssetsUpToDate(g_InstallDir)) {
                g_UI.ShowScreen(LauncherScreen::Main);
                g_UI.SetUsername(g_Tokens.username);
            } else {
                g_UI.ShowScreen(LauncherScreen::Download);
                g_UI.SetDownloadProgress(0.0f, "Checking assets...");
                std::thread(AssetSyncThread).detach();
            }
            autoLoggedIn = true;
        }
    }

    if (!autoLoggedIn) {
        g_UI.ShowScreen(LauncherScreen::Login);
    }

    // Main loop
    while (g_Running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                g_Running = false;
                break;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                g_Width = event.window.data1;
                g_Height = event.window.data2;
                bgfx::reset(g_Width, g_Height, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, g_Width, g_Height);
                g_UI.SetViewportSize(g_Width, g_Height);
            }
            g_UI.ProcessEvent(event);
        }

        // Process status messages from background threads
        {
            std::string statusMsg;
            while (PopStatus(statusMsg)) {
                if (g_UI.GetCurrentScreen() == LauncherScreen::Login)
                    g_UI.SetStatusText(statusMsg);
                else if (g_UI.GetCurrentScreen() == LauncherScreen::Download)
                    g_UI.SetDownloadProgress(0.0f, statusMsg);
            }
        }

        // Check if background auth completed successfully
        if (g_AuthSuccess.exchange(false)) {
            {
                std::lock_guard<std::mutex> lock(g_TokenMutex);
                g_Tokens = g_PendingTokens;
            }
            // Start asset sync, show download screen
            g_UI.ShowScreen(LauncherScreen::Download);
            g_UI.SetDownloadProgress(0.0f, "Checking assets...");
            std::thread(AssetSyncThread).detach();
        }

        // Check if auth failed
        if (g_AuthFailed.exchange(false)) {
            g_UI.ShowScreen(LauncherScreen::Login);
        }

        // Check if asset sync completed -> show main screen
        if (g_AssetsReady.exchange(false)) {
            g_UI.ShowScreen(LauncherScreen::Main);
            g_UI.SetUsername(g_Tokens.username);
        }

        bgfx::touch(0);
        g_UI.Update();
        g_UI.Render();
        bgfx::frame();
    }

    g_UI.Shutdown();
    bgfx::shutdown();
    SDL_DestroyWindow(g_Window);
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}
