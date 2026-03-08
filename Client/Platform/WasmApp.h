#pragma once
#ifdef __EMSCRIPTEN__

// ---------------------------------------------------------------------------
// WasmApp.h — Minimal stubs for CMinecraftApp / CConsoleMinecraftApp.
// Replaces Windows64_App.h + Consoles_App.h for WASM builds.
// Most methods are no-ops; only the ones called in active code paths
// have real implementations.
// ---------------------------------------------------------------------------

#include "WasmCompat.h"
#include "../Common/DLC/DLCManager.h"
#include "../Common/DLC/DLCCatalog.h"
#include "../Common/DLC/DLCInstaller.h"
#include "../Common/GameRules/GameRuleManager.h"
#include "../Common/GameRules/ConsoleGameRulesConstants.h"

class StringTable;
class C4JStringTable;
class DLCPack;

// Stub for platform-specific UIController (no-op for WASM)
struct CMinecraftUIController { void init() {} };

class CMinecraftApp
{
public:
    CMinecraftApp() : m_bGameStarted(false) {}
    virtual ~CMinecraftApp() {}

    // Game state
    bool GetGameStarted() const { return m_bGameStarted; }
    void SetGameStarted(bool b) { m_bGameStarted = b; }

    // Time
    void UpdateTime() {}

    // Media / strings (no-op for WASM — assets loaded via preload-file)
    void loadMediaArchive() {}
    void loadStringTable() {}
    void loadDefaultGameRules() {}

    // App lifecycle
    virtual void ExitGame() {}
    virtual void FatalLoadError() {}

    // DLC (minimal — stubs)
    DLCManager  m_dlcManager;
    DLCCatalog  m_dlcCatalog;
    DLCInstaller m_dlcInstaller;

    GameRuleManager m_gameRuleManager;

    // String table (returns null for WASM)
    C4JStringTable* GetStringTable() { return nullptr; }
    const wchar_t*  GetString(int) { return L""; }

    // Pause
    void SetAppPaused(bool) {}
    bool GetAppPaused() const { return false; }

    // Trial / full version (always full for WASM)
    bool IsFullVersion() const { return true; }

    // Misc stubs
    void DebugPrintf(const char*, ...) {}
    bool GetReallyChangingSessionType() const { return false; }
    void TickDLCOffersRetrieved() {}
    void TickTMSPPFilesRetrieved() {}
    void UpdateTrialPausedTimer() {}
    bool IntroRunning() const { return false; }
    void RunFrame() {}
    HRESULT Render() { return 0; }
    int GetGameSettings(int, int) const { return 0; }

    // Fake offer ID (used for profile init on Xbox — ignored for WASM)
    DWORD m_dwOfferID = 0;
    DWORD GAME_DEFINED_PROFILE_DATA_BYTES = 0;
    DWORD uiGameDefinedDataChangedBitmask = 0;

protected:
    bool m_bGameStarted;
};

class CConsoleMinecraftApp : public CMinecraftApp
{
public:
    CConsoleMinecraftApp() {}

    // Storage / TMS stubs
    void CaptureSaveThumbnail() {}
    void GetSaveThumbnail(PBYTE*, DWORD*) {}
    void ReleaseSaveThumbnail() {}
    void GetScreenshot(int, PBYTE*, DWORD*) {}
    int  LoadLocalTMSFile(WCHAR*) { return 0; }
    void FreeLocalTMSFiles(int) {}
    int  GetLocalTMSFileIndex(WCHAR*, bool, int) { return -1; }

    // Saving callbacks (no-op for WASM — handled by EmscriptenStorage)
    static void DisplaySavingMessage(void*) {}
    static void SignInChangeCallback(void*) {}
    static void NotificationsCallback(void*) {}
    static void DefaultOptionsCallback(void*) {}

    // Social / Rich presence stubs
    void SetRichPresenceContext(int, int) {}
    void StoreLaunchData() {}
    void TemporaryCreateGameStart() {}
};

extern CConsoleMinecraftApp app;

#endif // __EMSCRIPTEN__
