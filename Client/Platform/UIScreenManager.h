#pragma once
#include <string>
#include "BedrockUIParser.h"

class Minecraft;
namespace Rml { class ElementDocument; class Event; class EventListener; }

class UIScreenManager {
public:
    enum Screen {
        SCREEN_NONE,
        SCREEN_TITLE,
        SCREEN_IN_GAME,
        SCREEN_PAUSE,
        // game flow
        SCREEN_WORLD_SELECT,
        SCREEN_CREATE_WORLD,
        SCREEN_LOADING,
        SCREEN_DEATH,
        // settings
        SCREEN_SETTINGS,
        SCREEN_SETTINGS_AUDIO,
        SCREEN_SETTINGS_VIDEO,
        SCREEN_SETTINGS_CONTROLS,
        SCREEN_TEXTURE_PACKS,
        SCREEN_MOD_MANAGER,
        // containers
        SCREEN_INVENTORY,
        SCREEN_CRAFTING,
        SCREEN_FURNACE,
        SCREEN_BLAST_FURNACE,
        SCREEN_SMOKER,
        SCREEN_CHEST,
        SCREEN_DOUBLE_CHEST,
        SCREEN_ENDER_CHEST,
        SCREEN_BARREL,
        SCREEN_SHULKER,
        SCREEN_ENCHANTING,
        SCREEN_BREWING,
        SCREEN_ANVIL,
        SCREEN_DISPENSER,
        SCREEN_DROPPER,
        SCREEN_HOPPER,
        SCREEN_BEACON,
        SCREEN_HORSE,
        SCREEN_TRADING,
        SCREEN_GRINDSTONE,
        SCREEN_STONECUTTER,
        SCREEN_CARTOGRAPHY,
        SCREEN_LOOM,
        SCREEN_SMITHING,
        SCREEN_FABRICATOR,
        // special UI
        SCREEN_BOOK_EDIT,
        SCREEN_BOOK_READ,
        SCREEN_MAP,
        SCREEN_CREATIVE_SEARCH,
        SCREEN_ADVANCEMENTS,
        SCREEN_STATISTICS,
        SCREEN_COMMAND_BLOCK,
        SCREEN_SIGN_EDIT,
        // overlays
        SCREEN_CHAT,
        SCREEN_BED,
        SCREEN_PLAYER_LIST,
        // social/multiplayer
        SCREEN_SERVER_BROWSER,
        SCREEN_COMMUNITY_HUB,
        SCREEN_DRESSING_ROOM,
        SCREEN_FRIENDS,
        SCREEN_JOIN_GAME,
        SCREEN_HOST_GAME,
        SCREEN_CONNECTING,
        // extra
        SCREEN_CREDITS,
        SCREEN_HOW_TO_PLAY,
        SCREEN_EULA,
    };

    static UIScreenManager& Get();

    void Init(Minecraft* minecraft);
    void ShowScreen(Screen screen);
    Screen GetCurrentScreen() const { return m_currentScreen; }
    void HandleEvent(const std::string& buttonId);

    void ShowPauseMenu();
    void TogglePause();
    bool IsPaused() const { return m_currentScreen == SCREEN_PAUSE; }

    void HandleSliderChange(const std::string& trackId, float value);
    void PopulateAudioSettings();
    void PopulateVideoSettings();
    void PopulateControlsSettings();

    void OnKeyPressed(int vkCode);
    bool IsWaitingForKey() const { return m_waitingForKey; }

    // per-frame tick (connecting animation, credits scroll, etc.)
    void Tick();

    void SetLoadingProgress(float progress, const char* statusText);
    void SetDeathScore(int score);
    void RefreshWorldList();

    BedrockUIParser& GetParser() { return m_parser; }
    Minecraft* GetMinecraft() { return m_minecraft; }

    std::string ResolveUIPath(const char* filename);
    std::string GetThemeName();
    void ReloadCurrentScreen();

private:
    UIScreenManager() = default;
    ~UIScreenManager();

    void ShowTitleScreen();
    void ShowGenericScreen(const char* rmlFile, const char** buttonIds, int buttonCount);
    void HideCurrent();
    Rml::ElementDocument* LoadThemedDocument(const char* rmlFile);

    // track which screen we came from for "Back" buttons
    Screen m_previousScreen = SCREEN_NONE;

    Minecraft* m_minecraft = nullptr;
    Screen m_currentScreen = SCREEN_NONE;
    Rml::ElementDocument* m_currentDoc = nullptr;
    Rml::EventListener* m_buttonListener = nullptr;

    BedrockUIParser m_parser;
    bool m_parserInitialized = false;

    int m_selectedWorldIndex = -1;
    bool m_waitingForKey = false;
    int m_keyBindIndex = -1;

    int m_createGameMode = 1;    // 1=Creative default
    int m_createDifficulty = 0;
    bool m_createStructures = true;
    bool m_createBonusChest = false;

    int m_howToPlayPage = 0;
    int m_connectingTick = 0;

    void PopulateCreateWorldForm();
    void PopulateServerList();
    void PopulateHowToPlayPage();
};
