#if defined(__linux__) || defined(__APPLE__)

// ---------------------------------------------------------------------------
// UnixMain.cpp — Native entry point shared between Linux and macOS.
// Standard blocking game loop (no Emscripten restrictions).
// ---------------------------------------------------------------------------

#include "PlatformCompat.h"
#include "UnixApp.h"
#include "SDLWindow.h"
#include "SDLInput.h"
#include "SDLEventBridge.h"
#include "BgfxRenderer.h"
#include "RmlUIManager.h"
#include "UIScreenManager.h"

// Save paths differ per OS
#if defined(__APPLE__)
#  include <sys/types.h>
#  include <pwd.h>
static std::string GetSaveRoot() {
    const char* home = getenv("HOME");
    return std::string(home ? home : "") + "/Library/Application Support/OurCraft";
}
#else // Linux
static std::string GetSaveRoot() {
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/ourcraft";
    const char* home = getenv("HOME");
    return std::string(home ? home : "") + "/.local/share/ourcraft";
}
#endif

extern int g_iScreenWidth;
extern int g_iScreenHeight;

SDLWindow      g_SDLWindow;
BgfxRenderer   g_BgfxRenderer;
SDLEventBridge g_EventBridge;
// g_SDLInput defined in SDLInput.cpp

int g_iScreenWidth  = 1280;
int g_iScreenHeight = 720;

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int /*argc*/, char** /*argv*/)
{
    // Ensure save directory exists
    std::string saveRoot = GetSaveRoot();
    mkdir(saveRoot.c_str(), 0755);

    // --- Platform init ---
    if (!g_SDLWindow.Init("Minecraft", g_iScreenWidth, g_iScreenHeight))
    {
        fprintf(stderr, "[Unix] Failed to create SDL window\n");
        return 1;
    }

    if (!g_BgfxRenderer.Init(g_SDLWindow, g_iScreenWidth, g_iScreenHeight))
    {
        fprintf(stderr, "[Unix] Failed to init bgfx renderer\n");
        g_SDLWindow.Shutdown();
        return 1;
    }

    RmlUIManager::Get().Init(g_iScreenWidth, g_iScreenHeight);
    g_EventBridge.Init(&g_SDLWindow, &g_SDLInput, nullptr);

    // --- Game init ---
    Minecraft::main();
    app.SetGameStarted(true);

    // --- Standard game loop ---
    while (true)
    {
        if (!g_EventBridge.PumpEvents())
            break;

        g_iScreenWidth  = g_SDLWindow.GetWidth();
        g_iScreenHeight = g_SDLWindow.GetHeight();

        g_BgfxRenderer.StartFrame();
        app.UpdateTime();

        Minecraft* mc = Minecraft::GetInstance();
        if (mc && app.GetGameStarted())
        {
            mc->run_middle();
            mc->soundEngine->playMusicTick();
        }

        RmlUIManager::Get().Update();
        RmlUIManager::Get().Render();
        g_BgfxRenderer.Present();
    }

    // --- Cleanup ---
    RmlUIManager::Get().Shutdown();
    g_BgfxRenderer.Shutdown();
    g_SDLWindow.Shutdown();
    return 0;
}

#endif // linux || APPLE
