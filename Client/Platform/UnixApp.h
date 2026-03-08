#pragma once
#if defined(__linux__) || defined(__APPLE__)

// ---------------------------------------------------------------------------
// UnixApp.h — Minimal stubs for CMinecraftApp / CConsoleMinecraftApp.
// Shared between Linux and macOS builds.
// Mirrors WasmApp.h but without Emscripten-specific deps.
// ---------------------------------------------------------------------------

#include "PlatformCompat.h"
#include "../Common/DLC/DLCManager.h"
#include "../Common/DLC/DLCCatalog.h"
#include "../Common/DLC/DLCInstaller.h"
#include "../Common/GameRules/GameRuleManager.h"
#include "../Common/GameRules/ConsoleGameRulesConstants.h"

class StringTable;
class C4JStringTable;

class CMinecraftApp
{
public:
    CMinecraftApp() : m_bGameStarted(false) {}
    virtual ~CMinecraftApp() {}

    bool  GetGameStarted() const   { return m_bGameStarted; }
    void  SetGameStarted(bool b)   { m_bGameStarted = b; }

    void  UpdateTime()             {}
    void  loadMediaArchive()       {}
    void  loadStringTable()        {}
    void  loadDefaultGameRules()   {}

    virtual void ExitGame()        {}
    virtual void FatalLoadError()  {}

    DLCManager   m_dlcManager;
    DLCCatalog   m_dlcCatalog;
    DLCInstaller m_dlcInstaller;
    GameRuleManager m_gameRuleManager;

    C4JStringTable* GetStringTable()        { return nullptr; }
    const wchar_t*  GetString(int)          { return L""; }

    void SetAppPaused(bool)                 {}
    bool GetAppPaused() const               { return false; }
    bool IsFullVersion() const              { return true; }
    void DebugPrintf(const char*, ...)      {}
    bool GetReallyChangingSessionType() const { return false; }
    void TickDLCOffersRetrieved()           {}
    void TickTMSPPFilesRetrieved()          {}
    void UpdateTrialPausedTimer()           {}
    bool IntroRunning() const               { return false; }
    void RunFrame()                         {}
    HRESULT Render()                        { return 0; }
    int  GetGameSettings(int, int) const    { return 0; }

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

    void CaptureSaveThumbnail()              {}
    void GetSaveThumbnail(PBYTE*, DWORD*)    {}
    void ReleaseSaveThumbnail()              {}
    void GetScreenshot(int, PBYTE*, DWORD*)  {}
    int  LoadLocalTMSFile(WCHAR*)            { return 0; }
    void FreeLocalTMSFiles(int)              {}
    int  GetLocalTMSFileIndex(WCHAR*, bool, int) { return -1; }

    static void DisplaySavingMessage(void*)  {}
    static void SignInChangeCallback(void*)  {}
    static void NotificationsCallback(void*) {}
    static void DefaultOptionsCallback(void*){}

    void SetRichPresenceContext(int, int)    {}
    void StoreLaunchData()                   {}
    void TemporaryCreateGameStart()          {}
};

extern CConsoleMinecraftApp app;

#endif // linux || APPLE
