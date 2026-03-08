#ifdef __EMSCRIPTEN__

// EmscriptenMain.cpp — WASM entry point and game loop
// mirrors Windows64/Windows64_OurCraft.cpp but uses emscripten_set_main_loop
// instead of a blocking while() loop

#include <emscripten/emscripten.h>

#include "WasmCompat.h"
#include "WasmApp.h"
#include "SDLWindow.h"
#include "SDLInput.h"
#include "SDLEventBridge.h"
#include "BgfxRenderer.h"
#include "RmlUIManager.h"
#include "UIScreenManager.h"
#include "EmscriptenStorage.h"

class Minecraft;
extern void MinecraftWorld_RunStaticCtors();
extern int g_iScreenWidth;
extern int g_iScreenHeight;

// platform globals (same role as in Windows64_OurCraft.cpp)
SDLWindow      g_SDLWindow;
BgfxRenderer   g_BgfxRenderer;
SDLEventBridge g_EventBridge;
// g_SDLInput defined in SDLInput.cpp
// g_KBMInput not used for WASM

int g_iScreenWidth  = 1280;
int g_iScreenHeight = 720;

namespace { Minecraft* s_mc = nullptr; }

static void WasmTickFrame()
{
    if (!g_EventBridge.PumpEvents())
    {
        emscripten_cancel_main_loop();
        RmlUIManager::Get().Shutdown();
        g_BgfxRenderer.Shutdown();
        g_SDLWindow.Shutdown();
        return;
    }

    g_iScreenWidth  = g_SDLWindow.GetWidth();
    g_iScreenHeight = g_SDLWindow.GetHeight();

    g_BgfxRenderer.StartFrame();

    app.UpdateTime();

    s_mc = Minecraft::GetInstance();
    if (s_mc && app.GetGameStarted())
    {
        s_mc->run_middle();
        s_mc->soundEngine->playMusicTick();
    }

    RmlUIManager::Get().Update();
    RmlUIManager::Get().Render();

    g_BgfxRenderer.Present();
}

int main(int /*argc*/, char** /*argv*/)
{
    EmscriptenStorage::Init();

    if (!g_SDLWindow.Init("Minecraft", g_iScreenWidth, g_iScreenHeight))
    {
        printf("[WASM] Failed to create SDL window\n");
        return 1;
    }

    if (!g_BgfxRenderer.Init(g_SDLWindow, g_iScreenWidth, g_iScreenHeight))
    {
        printf("[WASM] Failed to init bgfx renderer\n");
        g_SDLWindow.Shutdown();
        return 1;
    }

    RmlUIManager::Get().Init(g_iScreenWidth, g_iScreenHeight);

    g_EventBridge.Init(&g_SDLWindow, &g_SDLInput, nullptr);

    Minecraft::main();

    app.SetGameStarted(true);

    // 0 fps = use requestAnimationFrame (vsync); 1 = don't return from main
    emscripten_set_main_loop(WasmTickFrame, 0, 1);

    return 0;
}

#endif // __EMSCRIPTEN__
