#include "UIScreenManager.h"
static std::string ResolveLogoPath();
#include "RmlUIManager.h"
#include "..\World\Math\Language.h"
#include "../Input/KeyboardMouseInput.h"
#include <windows.h>
#include "HUDOverlay.h"
#include "InventoryOverlay.h"
#include "CraftingOverlay.h"
#include "FurnaceOverlay.h"
#include "ChestOverlay.h"
#include "EnchantingOverlay.h"
#include "BrewingOverlay.h"
#include "AnvilOverlay.h"
#include "DispenserOverlay.h"
#include "HopperOverlay.h"
#include "BeaconOverlay.h"
#include "HorseOverlay.h"
#include "TradingOverlay.h"
#include "GrindstoneOverlay.h"
#include "StonecutterOverlay.h"
#include "CartographyOverlay.h"
#include "LoomOverlay.h"
#include "SmithingOverlay.h"
#include "FabricatorOverlay.h"
#include "CreativeOverlay.h"
#include "BedrockUIParser.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <direct.h> // _getcwd
#include <cstdio>
#include <sys/stat.h>

// forward declare - Minecraft.h pulls in too much via stdafx
class Minecraft;
extern void UIScreenManager_AutoStartWorld(Minecraft* mc);
extern void UIScreenManager_CreateWorld(Minecraft* mc, const char* name, int seed, int gameMode, int difficulty, int levelType, bool structures, bool bonusChest);
extern void UIScreenManager_LoadWorld(Minecraft* mc, int worldIndex);
extern void UIScreenManager_DeleteWorld(Minecraft* mc, int worldIndex);
extern void UIScreenManager_StopGame(Minecraft* mc);
extern void UIScreenManager_PerformSave(Minecraft* mc);
extern void UIScreenManager_ReleaseLevel(Minecraft* mc);
extern void UIScreenManager_Respawn(Minecraft* mc);
extern int  UIScreenManager_GetWorldCount(Minecraft* mc);
extern const char* UIScreenManager_GetWorldName(Minecraft* mc, int index);

// routes button clicks to UIScreenManager
class ButtonClickListener : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override {
        Rml::Element* el = event.GetCurrentElement();
        if (!el) return;

        std::string id = el->GetId();

        // slider track click — calculate percentage from mouse position
        if (id.size() > 6 && id.substr(0, 6) == "track_") {
            float mouseX = event.GetParameter<float>("mouse_x", 0.0f);
            Rml::Vector2f absPos = el->GetAbsoluteOffset(Rml::BoxArea::Content);
            float trackWidth = el->GetBox().GetSize(Rml::BoxArea::Content).x;
            float pct = 0.5f;
            if (trackWidth > 0) {
                pct = (mouseX - absPos.x) / trackWidth;
                if (pct < 0.0f) pct = 0.0f;
                if (pct > 1.0f) pct = 1.0f;
            }
            UIScreenManager::Get().HandleSliderChange(id, pct);
            return;
        }

        UIScreenManager::Get().HandleEvent(id);
    }
};

UIScreenManager& UIScreenManager::Get() {
    static UIScreenManager instance;
    return instance;
}

UIScreenManager::~UIScreenManager() {
    delete m_buttonListener;
    m_buttonListener = nullptr;
}

void UIScreenManager::Init(Minecraft* minecraft) {
    m_minecraft = minecraft;
    if (!m_buttonListener) {
        m_buttonListener = new ButtonClickListener();
    }
}

Rml::ElementDocument* UIScreenManager::LoadThemedDocument(const char* rmlFile) {
    auto* context = RmlUIManager::Get().GetContext();
    if (!context) return nullptr;

    FILE* f = fopen(rmlFile, "rb");
    if (!f) return context->LoadDocument(rmlFile); // fallback

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);

    // inject theme CSS link before </head>
    std::string themeCssLink = "<link type=\"text/rcss\" href=\"Data/ui/theme_" + GetThemeName() + ".rcss\"/>\n";
    size_t headEnd = content.find("</head>");
    if (headEnd != std::string::npos) {
        content.insert(headEnd, themeCssLink);
    }

    // translation substitution: replace {{key}} with translated value
    {
        Language* lang = Language::getInstance();
        std::string result;
        result.reserve(content.size());
        size_t pos = 0;
        while (pos < content.size()) {
            size_t open = content.find("{{", pos);
            if (open == std::string::npos) {
                result.append(content, pos, std::string::npos);
                break;
            }
            result.append(content, pos, open - pos);
            size_t close = content.find("}}", open + 2);
            if (close == std::string::npos) {
                result.append(content, open, std::string::npos);
                break;
            }
            std::string key = content.substr(open + 2, close - open - 2);
            std::wstring wkey(key.begin(), key.end());
            std::wstring wval = lang->getElement(wkey);
            if (wval == wkey) {
                // no translation found — use key as fallback
                result += key;
            } else {
                int utf8len = WideCharToMultiByte(CP_UTF8, 0, wval.c_str(), (int)wval.size(), nullptr, 0, nullptr, nullptr);
                if (utf8len > 0) {
                    std::string utf8val(utf8len, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, wval.c_str(), (int)wval.size(), &utf8val[0], utf8len, nullptr, nullptr);
                    result += utf8val;
                } else {
                    result += key;
                }
            }
            pos = close + 2;
        }
        content = std::move(result);
    }

    return context->LoadDocumentFromMemory(content, rmlFile);
}

void UIScreenManager::ShowScreen(Screen screen) {
    // track previous screen for Back buttons
    if (m_currentScreen != SCREEN_NONE && m_currentScreen != screen) {
        m_previousScreen = m_currentScreen;
    }
    HideCurrent();
    m_currentScreen = screen;

    // mouse grab: release when any UI overlay is shown, re-grab when returning to game
    if (screen == SCREEN_IN_GAME || screen == SCREEN_NONE) {
        if (g_KBMInput.IsWindowFocused())
            g_KBMInput.SetMouseGrabbed(true);
    } else {
        g_KBMInput.SetMouseGrabbed(false);
    }

    switch (screen) {
        case SCREEN_TITLE:      ShowTitleScreen(); break;
        case SCREEN_PAUSE:      ShowPauseMenu(); break;
        case SCREEN_WORLD_SELECT: {
            const char* ids[] = { "btn_play_world", "btn_create_world", "btn_delete_world", "btn_back" };
            ShowGenericScreen(ResolveUIPath("world_select.rml").c_str(), ids, 4);
            RefreshWorldList();
            break;
        }
        case SCREEN_CREATE_WORLD: {
            const char* ids[] = {
                "btn_create", "btn_cancel",
                "btn_cycle_gamemode", "btn_cycle_difficulty",
                "btn_toggle_structures", "btn_toggle_bonus"
            };
            ShowGenericScreen(ResolveUIPath("create_world.rml").c_str(), ids, 6);
            PopulateCreateWorldForm();
            break;
        }
        case SCREEN_LOADING: {
            ShowGenericScreen(ResolveUIPath("loading_screen.rml").c_str(), nullptr, 0);
            break;
        }
        case SCREEN_SETTINGS: {
            const char* ids[] = { "btn_settings_audio", "btn_settings_video", "btn_settings_controls", "btn_cycle_language", "btn_settings_texture_packs", "btn_settings_mods", "btn_cycle_theme", "btn_settings_back" };
            ShowGenericScreen(ResolveUIPath("settings.rml").c_str(), ids, 8);
            if (m_currentDoc) {
                if (auto* el = m_currentDoc->GetElementById("btn_cycle_theme")) {
                    std::string name = GetThemeName();
                    name[0] = toupper(name[0]);
                    el->SetInnerRML(("UI Theme: " + name).c_str());
                }
                if (auto* el = m_currentDoc->GetElementById("btn_cycle_language")) {
                    extern const char* UIScreenManager_GetLanguage(Minecraft*);
                    std::string curLang = m_minecraft ? UIScreenManager_GetLanguage(m_minecraft) : "en_US";
                    std::wstring wlabel = Language::getInstance()->getElement(L"options.language");
                    std::string label(wlabel.begin(), wlabel.end());
                    el->SetInnerRML((label + ": " + curLang).c_str());
                }
            }
            break;
        }
        case SCREEN_SETTINGS_AUDIO: {
            const char* ids[] = { "btn_audio_back", "track_music", "track_sound" };
            ShowGenericScreen(ResolveUIPath("settings_audio.rml").c_str(), ids, 3);
            PopulateAudioSettings();
            break;
        }
        case SCREEN_SETTINGS_VIDEO: {
            const char* ids[] = {
                "btn_video_back",
                "btn_cycle_render_dist", "btn_cycle_graphics",
                "btn_cycle_gui_scale", "btn_cycle_particles",
                "btn_cycle_clouds", "btn_cycle_ao",
                "btn_cycle_window", "btn_cycle_fps",
                "track_fov", "track_gamma"
            };
            ShowGenericScreen(ResolveUIPath("settings_video.rml").c_str(), ids, 11);
            PopulateVideoSettings();
            break;
        }
        case SCREEN_SETTINGS_CONTROLS: {
            const char* ids[] = {
                "btn_controls_back", "btn_toggle_invert_y", "track_sensitivity",
                "keybind_0", "keybind_1", "keybind_2", "keybind_3",
                "keybind_4", "keybind_5", "keybind_6", "keybind_7",
                "keybind_8", "keybind_9", "keybind_10", "keybind_11",
                "keybind_12", "keybind_13"
            };
            ShowGenericScreen(ResolveUIPath("settings_controls.rml").c_str(), ids, 17);
            m_waitingForKey = false;
            m_keyBindIndex = -1;
            PopulateControlsSettings();
            break;
        }
        case SCREEN_DEATH: {
            const char* ids[] = { "btn_respawn", "btn_death_quit" };
            ShowGenericScreen(ResolveUIPath("death_screen.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_FURNACE:
        case SCREEN_BLAST_FURNACE:
        case SCREEN_SMOKER:
            HUDOverlay::Get().Hide();
            FurnaceOverlay::Get().Show("furnace.rml");
            break;
        case SCREEN_CHEST:
        case SCREEN_DOUBLE_CHEST:
        case SCREEN_ENDER_CHEST:
        case SCREEN_BARREL:
        case SCREEN_SHULKER:
            HUDOverlay::Get().Hide();
            ChestOverlay::Get().Show("chest.rml");
            break;
        case SCREEN_ENCHANTING:
            HUDOverlay::Get().Hide();
            EnchantingOverlay::Get().Show("enchanting.rml");
            break;
        case SCREEN_BREWING:
            HUDOverlay::Get().Hide();
            BrewingOverlay::Get().Show("brewing_stand.rml");
            break;
        case SCREEN_ANVIL:
            HUDOverlay::Get().Hide();
            AnvilOverlay::Get().Show("anvil.rml");
            break;
        case SCREEN_DISPENSER:
        case SCREEN_DROPPER:
            HUDOverlay::Get().Hide();
            DispenserOverlay::Get().Show("dispenser.rml");
            break;
        case SCREEN_HOPPER:
            HUDOverlay::Get().Hide();
            HopperOverlay::Get().Show("hopper.rml");
            break;
        case SCREEN_BEACON:
            HUDOverlay::Get().Hide();
            BeaconOverlay::Get().Show("beacon.rml");
            break;
        case SCREEN_HORSE:
            HUDOverlay::Get().Hide();
            HorseOverlay::Get().Show("horse.rml");
            break;
        case SCREEN_TRADING:
            HUDOverlay::Get().Hide();
            TradingOverlay::Get().Show("trading.rml");
            break;
        case SCREEN_GRINDSTONE:
            HUDOverlay::Get().Hide();
            GrindstoneOverlay::Get().Show("grindstone.rml");
            break;
        case SCREEN_STONECUTTER:
            HUDOverlay::Get().Hide();
            StonecutterOverlay::Get().Show("stonecutter.rml");
            break;
        case SCREEN_CARTOGRAPHY:
            HUDOverlay::Get().Hide();
            CartographyOverlay::Get().Show("cartography.rml");
            break;
        case SCREEN_LOOM:
            HUDOverlay::Get().Hide();
            LoomOverlay::Get().Show("loom.rml");
            break;
        case SCREEN_SMITHING:
            HUDOverlay::Get().Hide();
            SmithingOverlay::Get().Show("smithing_table.rml");
            break;
        case SCREEN_FABRICATOR:
            HUDOverlay::Get().Hide();
            FabricatorOverlay::Get().Show("fabricator.rml");
            break;
        case SCREEN_JOIN_GAME: {
            const char* ids[] = { "btn_join", "btn_join_back" };
            ShowGenericScreen(ResolveUIPath("join_game.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_HOST_GAME: {
            const char* ids[] = { "btn_host_start", "btn_host_back" };
            ShowGenericScreen(ResolveUIPath("host_game.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_CONNECTING: {
            const char* ids[] = { "btn_connect_cancel" };
            ShowGenericScreen(ResolveUIPath("connecting.rml").c_str(), ids, 1);
            break;
        }
        case SCREEN_CREDITS: {
            const char* ids[] = { "btn_credits_back" };
            ShowGenericScreen(ResolveUIPath("credits.rml").c_str(), ids, 1);
            break;
        }
        case SCREEN_HOW_TO_PLAY: {
            const char* ids[] = { "btn_howtoplay_back", "btn_howtoplay_prev", "btn_howtoplay_next" };
            ShowGenericScreen(ResolveUIPath("how_to_play.rml").c_str(), ids, 3);
            m_howToPlayPage = 0;
            PopulateHowToPlayPage();
            break;
        }
        case SCREEN_EULA: {
            const char* ids[] = { "btn_eula_accept", "btn_eula_decline" };
            ShowGenericScreen(ResolveUIPath("eula.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_TEXTURE_PACKS: {
            const char* ids[] = { "btn_tp_back", "btn_tp_enable", "btn_tp_disable" };
            ShowGenericScreen(ResolveUIPath("texture_packs.rml").c_str(), ids, 3);
            break;
        }
        case SCREEN_MOD_MANAGER: {
            const char* ids[] = { "btn_mod_back", "btn_mod_enable", "btn_mod_disable" };
            ShowGenericScreen(ResolveUIPath("mod_manager.rml").c_str(), ids, 3);
            break;
        }
        case SCREEN_SERVER_BROWSER: {
            const char* ids[] = { "btn_server_back", "btn_server_join", "btn_server_direct", "btn_server_add" };
            ShowGenericScreen(ResolveUIPath("server_browser.rml").c_str(), ids, 4);
            PopulateServerList();
            break;
        }
        case SCREEN_INVENTORY:
            HUDOverlay::Get().Hide();
            InventoryOverlay::Get().Show();
            break;
        case SCREEN_CRAFTING:
            HUDOverlay::Get().Hide();
            CraftingOverlay::Get().Show();
            break;
        case SCREEN_CHAT: {
            const char* ids[] = { "btn_chat_send", "btn_chat_close" };
            ShowGenericScreen(ResolveUIPath("chat_input.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_CREATIVE_SEARCH:
            HUDOverlay::Get().Hide();
            CreativeOverlay::Get().ShowCreative();
            break;
        case SCREEN_BOOK_EDIT: {
            const char* ids[] = {
                "btn_book_prev", "btn_book_next", "btn_book_sign", "btn_book_done"
            };
            ShowGenericScreen(ResolveUIPath("book_edit.rml").c_str(), ids, 4);
            break;
        }
        case SCREEN_BOOK_READ: {
            const char* ids[] = {
                "btn_book_prev", "btn_book_next", "btn_book_close"
            };
            ShowGenericScreen(ResolveUIPath("book_read.rml").c_str(), ids, 3);
            break;
        }
        case SCREEN_SIGN_EDIT: {
            const char* ids[] = { "btn_sign_done" };
            ShowGenericScreen(ResolveUIPath("sign_edit.rml").c_str(), ids, 1);
            break;
        }
        case SCREEN_COMMAND_BLOCK: {
            const char* ids[] = {
                "btn_cmd_mode", "btn_cmd_conditional", "btn_cmd_always",
                "btn_cmd_done", "btn_cmd_cancel"
            };
            ShowGenericScreen(ResolveUIPath("command_block.rml").c_str(), ids, 5);
            break;
        }
        case SCREEN_MAP: {
            const char* ids[] = { "btn_map_close" };
            ShowGenericScreen(ResolveUIPath("map.rml").c_str(), ids, 1);
            break;
        }
        case SCREEN_PLAYER_LIST: {
            ShowGenericScreen(ResolveUIPath("player_list.rml").c_str(), nullptr, 0);
            break;
        }
        case SCREEN_BED: {
            const char* ids[] = { "btn_sleep", "btn_bed_leave" };
            ShowGenericScreen(ResolveUIPath("bed.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_ADVANCEMENTS: {
            const char* ids[] = {
                "btn_adv_story", "btn_adv_nether", "btn_adv_end",
                "btn_adv_adventure", "btn_adv_husbandry", "btn_adv_close"
            };
            ShowGenericScreen(ResolveUIPath("advancements.rml").c_str(), ids, 6);
            break;
        }
        case SCREEN_STATISTICS: {
            const char* ids[] = {
                "btn_stat_general", "btn_stat_blocks", "btn_stat_items",
                "btn_stat_mobs", "btn_stat_close"
            };
            ShowGenericScreen(ResolveUIPath("statistics.rml").c_str(), ids, 5);
            break;
        }
        case SCREEN_FRIENDS: {
            const char* ids[] = { "btn_friends_invite", "btn_friends_back" };
            ShowGenericScreen(ResolveUIPath("friends.rml").c_str(), ids, 2);
            break;
        }
        case SCREEN_DRESSING_ROOM: {
            const char* ids[] = { "btn_skin_select", "btn_skin_import", "btn_skin_back" };
            ShowGenericScreen(ResolveUIPath("dressing_room.rml").c_str(), ids, 3);
            break;
        }
        case SCREEN_COMMUNITY_HUB: {
            const char* ids[] = {
                "btn_hub_mods", "btn_hub_textures", "btn_hub_skins", "btn_hub_maps",
                "btn_hub_back"
            };
            ShowGenericScreen(ResolveUIPath("community_hub.rml").c_str(), ids, 5);
            break;
        }
        case SCREEN_NONE:
        case SCREEN_IN_GAME:
        default:
            break;
    }
}

void UIScreenManager::HideCurrent() {
    if (m_currentDoc) {
        m_currentDoc->Close();
        m_currentDoc = nullptr;
    }
}

void UIScreenManager::ShowTitleScreen() {
    if (!RmlUIManager::Get().GetContext()) {
        printf("[UIScreenManager] ERROR: No RmlUi context\n");
        return;
    }

    // EULA gate: show EULA screen first if not yet accepted
    if (m_minecraft) {
        extern bool UIScreenManager_GetEulaAccepted(Minecraft*);
        if (!UIScreenManager_GetEulaAccepted(m_minecraft)) {
            m_currentScreen = SCREEN_EULA;
            const char* eulaIds[] = { "btn_eula_accept", "btn_eula_decline" };
            ShowGenericScreen(ResolveUIPath("eula.rml").c_str(), eulaIds, 2);
            return;
        }
    }

    printf("[UIScreenManager] Loading start_screen.rml...\n");
    m_currentDoc = LoadThemedDocument(ResolveUIPath("start_screen.rml").c_str());
    if (!m_currentDoc) {
        printf("[UIScreenManager] ERROR: Failed to load start_screen.rml\n");
        char cwd[512];
        if (_getcwd(cwd, sizeof(cwd)))
            printf("[UIScreenManager] Working directory: %s\n", cwd);
        return;
    }

    m_currentDoc->Show();
    printf("[UIScreenManager] Title screen loaded OK\n");

    // Apply logo: resource pack override or Data/ui/logo.png
    if (Rml::Element* logoImg = m_currentDoc->GetElementById("logo-img")) {
        std::string logoSrc = ResolveLogoPath();
        logoImg->SetAttribute("src", logoSrc);
        printf("[UIScreenManager] Logo: %s\n", logoSrc.c_str());
    }

    const char* buttonIds[] = {
        "btn_play", "btn_multiplayer", "btn_options", "btn_marketplace",
        "btn_signin", "btn_profile", "btn_dressing"
    };
    for (const char* id : buttonIds) {
        if (Rml::Element* el = m_currentDoc->GetElementById(id)) {
            el->AddEventListener(Rml::EventId::Click, m_buttonListener);
        }
    }
}

void UIScreenManager::ShowGenericScreen(const char* rmlFile, const char** buttonIds, int buttonCount) {
    m_currentDoc = LoadThemedDocument(rmlFile);
    if (!m_currentDoc) {
        SDL_Log("[UIScreenManager] Failed to load %s", rmlFile);
        return;
    }

    m_currentDoc->Show();

    for (int i = 0; i < buttonCount; i++) {
        if (Rml::Element* el = m_currentDoc->GetElementById(buttonIds[i])) {
            el->AddEventListener(Rml::EventId::Click, m_buttonListener);
        }
    }

    SDL_Log("[UIScreenManager] Loaded %s", rmlFile);
}

void UIScreenManager::ShowPauseMenu() {
    m_currentDoc = LoadThemedDocument(ResolveUIPath("pause_menu.rml").c_str());
    if (!m_currentDoc) {
        SDL_Log("[UIScreenManager] Failed to load pause_menu.rml");
        return;
    }

    m_currentDoc->Show();

    const char* buttonIds[] = { "btn_resume", "btn_save", "btn_options", "btn_quit_title" };
    for (const char* id : buttonIds) {
        if (Rml::Element* el = m_currentDoc->GetElementById(id)) {
            el->AddEventListener(Rml::EventId::Click, m_buttonListener);
        }
    }

    SDL_Log("[UIScreenManager] Pause menu shown");
}

void UIScreenManager::TogglePause() {
    if (m_currentScreen == SCREEN_PAUSE) {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
        SDL_Log("[UIScreenManager] Resumed game");
    } else if (m_currentScreen == SCREEN_IN_GAME) {
        HUDOverlay::Get().Hide();
        ShowScreen(SCREEN_PAUSE);
        SDL_Log("[UIScreenManager] Paused game");
    }
}

void UIScreenManager::SetLoadingProgress(float progress, const char* statusText) {
    if (!m_currentDoc || m_currentScreen != SCREEN_LOADING) return;

    if (auto* fill = m_currentDoc->GetElementById("progress_fill")) {
        char style[64];
        snprintf(style, sizeof(style), "width: %.0f%%;", progress * 100.0f);
        fill->SetAttribute("style", style);
    }
    if (statusText) {
        if (auto* status = m_currentDoc->GetElementById("loading_status")) {
            status->SetInnerRML(statusText);
        }
    }
}

void UIScreenManager::SetDeathScore(int score) {
    if (!m_currentDoc || m_currentScreen != SCREEN_DEATH) return;
    if (auto* el = m_currentDoc->GetElementById("death_score")) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Score: %d", score);
        el->SetInnerRML(buf);
    }
}

void UIScreenManager::RefreshWorldList() {
    if (!m_currentDoc || m_currentScreen != SCREEN_WORLD_SELECT) return;
    auto* list = m_currentDoc->GetElementById("world_list");
    if (!list) return;

    while (list->GetNumChildren() > 0)
        list->RemoveChild(list->GetChild(0));

    if (!m_minecraft) return;

    int count = UIScreenManager_GetWorldCount(m_minecraft);
    for (int i = 0; i < count; i++) {
        const char* name = UIScreenManager_GetWorldName(m_minecraft, i);
        char idBuf[32];
        snprintf(idBuf, sizeof(idBuf), "world_%d", i);

        Rml::ElementPtr item = list->GetOwnerDocument()->CreateElement("div");
        item->SetAttribute("id", idBuf);
        item->SetAttribute("class", "list-item");
        item->SetInnerRML(name ? name : "Unknown World");
        item->AddEventListener(Rml::EventId::Click, m_buttonListener);
        list->AppendChild(std::move(item));
    }

    if (count == 0) {
        Rml::ElementPtr empty = list->GetOwnerDocument()->CreateElement("div");
        empty->SetAttribute("class", "subtitle-text");
        empty->SetInnerRML("No saved worlds found. Create a new world!");
        list->AppendChild(std::move(empty));
    }

    m_selectedWorldIndex = -1;
    SDL_Log("[UIScreenManager] World list refreshed: %d worlds", count);
}

void UIScreenManager::PopulateCreateWorldForm() {
    if (!m_currentDoc) return;

    static const char* gameModes[] = { "Mode: Survival", "Mode: Creative", "Mode: Adventure", "Mode: Spectator" };
    static const char* difficulties[] = { "Difficulty: Peaceful", "Difficulty: Easy", "Difficulty: Normal", "Difficulty: Hard" };

    int gmIdx = m_createGameMode;
    if (gmIdx < 0 || gmIdx > 3) gmIdx = 1;
    if (auto* el = m_currentDoc->GetElementById("btn_cycle_gamemode"))
        el->SetInnerRML(gameModes[gmIdx]);

    int diffIdx = m_createDifficulty;
    if (diffIdx < 0 || diffIdx > 3) diffIdx = 0;
    if (auto* el = m_currentDoc->GetElementById("btn_cycle_difficulty"))
        el->SetInnerRML(difficulties[diffIdx]);

    if (auto* el = m_currentDoc->GetElementById("btn_toggle_structures"))
        el->SetInnerRML(m_createStructures ? "Structures: ON" : "Structures: OFF");

    if (auto* el = m_currentDoc->GetElementById("btn_toggle_bonus"))
        el->SetInnerRML(m_createBonusChest ? "Bonus Chest: ON" : "Bonus Chest: OFF");
}

void UIScreenManager::PopulateServerList() {
    if (!m_currentDoc) return;
    auto* list = m_currentDoc->GetElementById("server_list");
    if (!list) return;

    while (list->GetNumChildren() > 0)
        list->RemoveChild(list->GetChild(0));

    // servers.txt format: name|ip:port per line
    FILE* f = fopen("servers.txt", "r");
    int count = 0;
    if (f) {
        char lineBuf[256];
        while (fgets(lineBuf, sizeof(lineBuf), f)) {
            size_t len = strlen(lineBuf);
            while (len > 0 && (lineBuf[len-1] == '\n' || lineBuf[len-1] == '\r'))
                lineBuf[--len] = '\0';
            if (len == 0) continue;

            char* sep = strchr(lineBuf, '|');
            std::string name, addr;
            if (sep) {
                name = std::string(lineBuf, sep - lineBuf);
                addr = std::string(sep + 1);
            } else {
                name = lineBuf;
                addr = lineBuf;
            }

            char idBuf[32];
            snprintf(idBuf, sizeof(idBuf), "server_%d", count);

            Rml::ElementPtr item = list->GetOwnerDocument()->CreateElement("div");
            item->SetAttribute("id", idBuf);
            item->SetAttribute("class", "server-item");
            std::string inner = "<div class=\"server-name\">" + name + "</div>"
                                "<div class=\"server-addr\">" + addr + "</div>";
            item->SetInnerRML(inner.c_str());
            item->AddEventListener(Rml::EventId::Click, m_buttonListener);
            list->AppendChild(std::move(item));
            count++;
        }
        fclose(f);
    }

    if (count == 0) {
        Rml::ElementPtr empty = list->GetOwnerDocument()->CreateElement("div");
        empty->SetAttribute("class", "empty-label");
        empty->SetInnerRML("No servers. Add a server to get started.");
        list->AppendChild(std::move(empty));
    }

    SDL_Log("[UIScreenManager] Server list populated: %d servers", count);
}

static const char* s_howToPlayPages[] = {
    "Movement\n\nWASD - Move\nSpace - Jump\nShift - Sneak / Crouch\nCtrl - Sprint\nDouble-tap Space - Fly (Creative)",
    "Mining &amp; Building\n\nLeft Click - Break blocks / Attack\nRight Click - Place block / Use item\nMiddle Click - Pick Block (Creative)\nQ - Drop item",
    "Inventory\n\nE - Open Inventory\n1-9 - Select Hotbar Slot\nEsc - Pause Menu\nT - Open Chat\nF5 - Toggle Camera Perspective\n\nCraft items in the 2x2 inventory grid.\nUse a Crafting Table for 3x3 recipes."
};
static const int s_howToPlayPageCount = 3;

void UIScreenManager::PopulateHowToPlayPage() {
    if (!m_currentDoc) return;
    if (auto* el = m_currentDoc->GetElementById("howtoplay_content"))
        el->SetInnerRML(s_howToPlayPages[m_howToPlayPage]);
    if (auto* el = m_currentDoc->GetElementById("howtoplay_page_num")) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Page %d of %d", m_howToPlayPage + 1, s_howToPlayPageCount);
        el->SetInnerRML(buf);
    }
}

void UIScreenManager::Tick() {
    if (m_currentScreen == SCREEN_CONNECTING && m_currentDoc) {
        m_connectingTick++;
        if (auto* el = m_currentDoc->GetElementById("connecting_dots")) {
            int phase = (m_connectingTick / 20) % 3;
            if (phase == 0)      el->SetInnerRML(".");
            else if (phase == 1) el->SetInnerRML("..");
            else                 el->SetInnerRML("...");
        }
    }
}

void UIScreenManager::HandleSliderChange(const std::string& trackId, float value) {
    extern void UIScreenManager_SetMusicVolume(Minecraft*, float);
    extern void UIScreenManager_SetSoundVolume(Minecraft*, float);

    if (trackId == "track_music" && m_minecraft) {
        UIScreenManager_SetMusicVolume(m_minecraft, value);
    } else if (trackId == "track_sound" && m_minecraft) {
        UIScreenManager_SetSoundVolume(m_minecraft, value);
    } else if (trackId == "track_fov" && m_minecraft) {
        extern void UIScreenManager_SetFOV(Minecraft*, float);
        UIScreenManager_SetFOV(m_minecraft, value);
        PopulateVideoSettings();
    } else if (trackId == "track_gamma" && m_minecraft) {
        extern void UIScreenManager_SetGamma(Minecraft*, float);
        UIScreenManager_SetGamma(m_minecraft, value);
        PopulateVideoSettings();
    } else if (trackId == "track_sensitivity" && m_minecraft) {
        extern void UIScreenManager_SetSensitivity(Minecraft*, float);
        UIScreenManager_SetSensitivity(m_minecraft, value);
        PopulateControlsSettings();
    }

    PopulateAudioSettings();
}

void UIScreenManager::PopulateAudioSettings() {
    if (!m_currentDoc || !m_minecraft) return;
    extern float UIScreenManager_GetMusicVolume(Minecraft*);
    extern float UIScreenManager_GetSoundVolume(Minecraft*);

    float music = UIScreenManager_GetMusicVolume(m_minecraft);
    float sound = UIScreenManager_GetSoundVolume(m_minecraft);

    auto setSlider = [&](const char* fillId, const char* valId, float val) {
        if (auto* fill = m_currentDoc->GetElementById(fillId)) {
            char style[64];
            snprintf(style, sizeof(style), "width: %.0f%%;", val * 100.0f);
            fill->SetAttribute("style", style);
        }
        if (auto* valEl = m_currentDoc->GetElementById(valId)) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f%%", val * 100.0f);
            valEl->SetInnerRML(buf);
        }
    };
    setSlider("fill_music", "val_music", music);
    setSlider("fill_sound", "val_sound", sound);
}

void UIScreenManager::PopulateVideoSettings() {
    if (!m_currentDoc || !m_minecraft) return;

    extern const char* UIScreenManager_GetVideoOption(Minecraft*, int);
    extern float UIScreenManager_GetFOV(Minecraft*);
    extern float UIScreenManager_GetGamma(Minecraft*);
    extern const char* UIScreenManager_GetWindowModeStr(Minecraft*);
    extern const char* UIScreenManager_GetTargetFPSStr(Minecraft*);

    // returns "Caption: Value"
    auto setBtn = [&](const char* btnId, int optId) {
        if (auto* el = m_currentDoc->GetElementById(btnId)) {
            el->SetInnerRML(UIScreenManager_GetVideoOption(m_minecraft, optId));
        }
    };
    setBtn("btn_cycle_render_dist", 4);   // RENDER_DISTANCE
    setBtn("btn_cycle_graphics", 10);     // GRAPHICS
    setBtn("btn_cycle_gui_scale", 12);    // GUI_SCALE
    setBtn("btn_cycle_particles", 16);    // PARTICLES
    setBtn("btn_cycle_clouds", 15);       // RENDER_CLOUDS
    setBtn("btn_cycle_ao", 11);           // AMBIENT_OCCLUSION

    if (auto* el = m_currentDoc->GetElementById("btn_cycle_window")) {
        std::string text = std::string("Window Mode: ") + UIScreenManager_GetWindowModeStr(m_minecraft);
        el->SetInnerRML(text.c_str());
    }
    if (auto* el = m_currentDoc->GetElementById("btn_cycle_fps")) {
        std::string text = std::string("FPS Limit: ") + UIScreenManager_GetTargetFPSStr(m_minecraft);
        el->SetInnerRML(text.c_str());
    }

    float fov = UIScreenManager_GetFOV(m_minecraft);
    if (auto* fill = m_currentDoc->GetElementById("fill_fov")) {
        char style[64];
        snprintf(style, sizeof(style), "width: %.0f%%;", fov * 100.0f);
        fill->SetAttribute("style", style);
    }
    if (auto* valEl = m_currentDoc->GetElementById("val_fov")) {
        char buf[16];
        int fovDeg = (int)(70.0f + fov * 40.0f);
        snprintf(buf, sizeof(buf), "%d", fovDeg);
        valEl->SetInnerRML(buf);
    }

    float gamma = UIScreenManager_GetGamma(m_minecraft);
    if (auto* fill = m_currentDoc->GetElementById("fill_gamma")) {
        char style[64];
        snprintf(style, sizeof(style), "width: %.0f%%;", gamma * 100.0f);
        fill->SetAttribute("style", style);
    }
    if (auto* valEl = m_currentDoc->GetElementById("val_gamma")) {
        char buf[16];
        if (gamma <= 0.0f)
            snprintf(buf, sizeof(buf), "Moody");
        else if (gamma >= 1.0f)
            snprintf(buf, sizeof(buf), "Bright");
        else
            snprintf(buf, sizeof(buf), "+%.0f%%", gamma * 100.0f);
        valEl->SetInnerRML(buf);
    }
}

void UIScreenManager::PopulateControlsSettings() {
    if (!m_currentDoc || !m_minecraft) return;

    extern const char* UIScreenManager_GetKeyName(Minecraft*, int);
    extern float UIScreenManager_GetSensitivity(Minecraft*);
    extern bool UIScreenManager_GetInvertY(Minecraft*);

    for (int i = 0; i < 14; i++) {
        char id[32];
        snprintf(id, sizeof(id), "keybind_%d", i);
        if (auto* el = m_currentDoc->GetElementById(id)) {
            el->SetInnerRML(UIScreenManager_GetKeyName(m_minecraft, i));
        }
    }

    float sens = UIScreenManager_GetSensitivity(m_minecraft);
    if (auto* fill = m_currentDoc->GetElementById("fill_sensitivity")) {
        char style[64];
        snprintf(style, sizeof(style), "width: %.0f%%;", sens * 100.0f);
        fill->SetAttribute("style", style);
    }
    if (auto* valEl = m_currentDoc->GetElementById("val_sensitivity")) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", sens * 100.0f);
        valEl->SetInnerRML(buf);
    }

    bool inverted = UIScreenManager_GetInvertY(m_minecraft);
    if (auto* el = m_currentDoc->GetElementById("btn_toggle_invert_y")) {
        el->SetInnerRML(inverted ? "Invert Y: ON" : "Invert Y: OFF");
    }
}

void UIScreenManager::OnKeyPressed(int vkCode) {
    if (!m_waitingForKey || m_keyBindIndex < 0) return;

    extern void UIScreenManager_SetKeyBinding(Minecraft*, int, int);
    if (m_minecraft) UIScreenManager_SetKeyBinding(m_minecraft, m_keyBindIndex, vkCode);

    m_waitingForKey = false;
    m_keyBindIndex = -1;
    PopulateControlsSettings();
}

extern int UIScreenManager_GetUITheme(Minecraft* mc);

std::string UIScreenManager::GetThemeName() {
    if (!m_minecraft) return "bedrock";
    int theme = UIScreenManager_GetUITheme(m_minecraft);
    switch (theme) {
        case 1: return "java";
        case 2: return "console";
        default: return "bedrock";
    }
}

// Returns the img src (relative to Data/ui/) for the title logo.
// Scans resource packs for a logo.png override; falls back to Data/ui/logo.png.
static std::string ResolveLogoPath() {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("Data/resource_packs/*", &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            std::string pack = fd.cFileName;
            if (pack == "." || pack == ".." || pack == "vanilla") continue;
            std::string candidate = "Data/resource_packs/" + pack + "/logo.png";
            struct stat st;
            if (stat(candidate.c_str(), &st) == 0) {
                FindClose(h);
                return "../resource_packs/" + pack + "/logo.png";
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return "logo.png"; // Data/ui/logo.png
}

std::string UIScreenManager::ResolveUIPath(const char* filename) {
    std::string theme = GetThemeName();

    // check theme override folder: Data/ui/<theme>/<filename>
    if (theme != "bedrock") {
        std::string overridePath = "Data/ui/" + theme + "/" + filename;
        struct stat st;
        if (stat(overridePath.c_str(), &st) == 0) {
            SDL_Log("[UIScreenManager] Using theme override: %s", overridePath.c_str());
            return overridePath;
        }
    }

    return std::string("Data/ui/") + filename;
}

void UIScreenManager::ReloadCurrentScreen() {
    Screen current = m_currentScreen;
    HideCurrent();
    m_currentScreen = SCREEN_NONE;
    ShowScreen(current);
}

void UIScreenManager::HandleEvent(const std::string& buttonId) {
    printf("[UIScreenManager] HandleEvent: '%s'\n", buttonId.c_str());

    if (buttonId == "btn_play" || buttonId == "play_button" || buttonId == "button.menu_play") {
        SDL_Log("[UIScreenManager] Play clicked - showing world select");
        HideCurrent();
        ShowScreen(SCREEN_WORLD_SELECT);
    }
    else if (buttonId == "btn_multiplayer") {
        SDL_Log("[UIScreenManager] Multiplayer clicked - showing server browser");
        ShowScreen(SCREEN_SERVER_BROWSER);
    }
    else if (buttonId == "btn_options" || buttonId == "settings_button" || buttonId == "button.menu_settings") {
        SDL_Log("[UIScreenManager] Settings clicked");
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_marketplace") {
        ShowScreen(SCREEN_COMMUNITY_HUB);
    }
    else if (buttonId == "btn_dressing") {
        ShowScreen(SCREEN_DRESSING_ROOM);
    }
    else if (buttonId == "btn_signin" || buttonId == "btn_profile") {
        SDL_Log("[UIScreenManager] %s clicked (not yet implemented)", buttonId.c_str());
    }
    else if (buttonId == "btn_quit" || buttonId == "button.menu_exit") {
        SDL_Log("[UIScreenManager] Quit clicked");
        HideCurrent();
        if (m_minecraft) UIScreenManager_StopGame(m_minecraft);
    }

    else if (buttonId.substr(0, 6) == "world_") {
        int idx = atoi(buttonId.c_str() + 6);
        m_selectedWorldIndex = idx;
        SDL_Log("[UIScreenManager] Selected world %d", idx);
        if (m_currentDoc) {
            if (auto* list = m_currentDoc->GetElementById("world_list")) {
                int childCount = list->GetNumChildren();
                for (int i = 0; i < childCount; i++) {
                    auto* child = list->GetChild(i);
                    if (!child) continue;
                    std::string childId = child->GetId();
                    if (childId.substr(0, 6) == "world_") {
                        child->SetAttribute("class", "list-item");
                    }
                }
            }
            char idBuf[32];
            snprintf(idBuf, sizeof(idBuf), "world_%d", idx);
            if (auto* el = m_currentDoc->GetElementById(idBuf)) {
                el->SetAttribute("class", "list-item selected");
            }
        }
    }
    else if (buttonId == "btn_play_world") {
        if (m_selectedWorldIndex >= 0 && m_minecraft) {
            SDL_Log("[UIScreenManager] Loading world %d", m_selectedWorldIndex);
            HideCurrent();
            m_currentScreen = SCREEN_IN_GAME;
            HUDOverlay::Get().Init();
            HUDOverlay::Get().Show();
            UIScreenManager_LoadWorld(m_minecraft, m_selectedWorldIndex);
        } else {
            SDL_Log("[UIScreenManager] No world selected");
        }
    }
    else if (buttonId == "btn_create_world") {
        SDL_Log("[UIScreenManager] Create New World clicked");
        ShowScreen(SCREEN_CREATE_WORLD);
    }
    else if (buttonId == "btn_back" && m_currentScreen == SCREEN_WORLD_SELECT) {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_delete_world") {
        if (m_selectedWorldIndex >= 0 && m_minecraft) {
            SDL_Log("[UIScreenManager] Deleting world %d", m_selectedWorldIndex);
            UIScreenManager_DeleteWorld(m_minecraft, m_selectedWorldIndex);
            m_selectedWorldIndex = -1;
            RefreshWorldList();
        }
    }

    else if (buttonId == "btn_create") {
        SDL_Log("[UIScreenManager] Create World confirmed");
        std::string worldName = "My World";
        int seed = 0;
        if (m_currentDoc) {
            if (auto* nameEl = m_currentDoc->GetElementById("input_name")) {
                Rml::String val = nameEl->GetAttribute<Rml::String>("value", "My World");
                worldName = std::string(val.c_str());
            }
            if (auto* seedEl = m_currentDoc->GetElementById("input_seed")) {
                Rml::String val = seedEl->GetAttribute<Rml::String>("value", "");
                if (!val.empty()) seed = atoi(val.c_str());
            }
        }
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Init();
        HUDOverlay::Get().Show();
        if (m_minecraft) {
            UIScreenManager_CreateWorld(m_minecraft, worldName.c_str(), seed,
                m_createGameMode, m_createDifficulty, 0,
                m_createStructures, m_createBonusChest);
        }
    }
    else if (buttonId == "btn_cancel") {
        ShowScreen(SCREEN_WORLD_SELECT);
    }
    else if (buttonId == "btn_cycle_gamemode") {
        m_createGameMode = (m_createGameMode + 1) % 4;
        PopulateCreateWorldForm();
    }
    else if (buttonId == "btn_cycle_difficulty") {
        m_createDifficulty = (m_createDifficulty + 1) % 4;
        PopulateCreateWorldForm();
    }
    else if (buttonId == "btn_toggle_structures") {
        m_createStructures = !m_createStructures;
        PopulateCreateWorldForm();
    }
    else if (buttonId == "btn_toggle_bonus") {
        m_createBonusChest = !m_createBonusChest;
        PopulateCreateWorldForm();
    }

    else if (buttonId == "btn_resume" || buttonId == "button.menu_continue") {
        TogglePause();
    }
    else if (buttonId == "btn_save") {
        SDL_Log("[UIScreenManager] Save clicked");
        if (m_minecraft) UIScreenManager_PerformSave(m_minecraft);
    }
    else if (buttonId == "btn_quit_title" || buttonId == "button.menu_quit") {
        SDL_Log("[UIScreenManager] Quit to Title clicked");
        HUDOverlay::Get().Shutdown();
        HideCurrent();
        if (m_minecraft) UIScreenManager_ReleaseLevel(m_minecraft);
        m_currentScreen = SCREEN_TITLE;
        ShowTitleScreen();
    }

    else if (buttonId == "btn_settings_audio") {
        ShowScreen(SCREEN_SETTINGS_AUDIO);
    }
    else if (buttonId == "btn_settings_video") {
        ShowScreen(SCREEN_SETTINGS_VIDEO);
    }
    else if (buttonId == "btn_settings_controls") {
        ShowScreen(SCREEN_SETTINGS_CONTROLS);
    }
    else if (buttonId == "btn_settings_language" || buttonId == "btn_cycle_language") {
        extern void UIScreenManager_SetLanguage(Minecraft* mc, const char* lang);
        static const char* languages[] = {
            "en_US", "en_GB", "fr_FR", "de_DE", "es_ES",
            "it_IT", "pt_BR", "ru_RU", "zh_CN", "ja_JP", "ko_KR"
        };
        static const int numLanguages = 11;
        std::string curLang = "en_US";
        if (m_minecraft) {
            extern const char* UIScreenManager_GetLanguage(Minecraft*);
            curLang = UIScreenManager_GetLanguage(m_minecraft);
        }
        int curIdx = 0;
        for (int i = 0; i < numLanguages; i++) {
            if (curLang == languages[i]) { curIdx = i; break; }
        }
        int nextIdx = (curIdx + 1) % numLanguages;
        if (m_minecraft) UIScreenManager_SetLanguage(m_minecraft, languages[nextIdx]);
        // reload settings screen to apply new translations
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_settings_texture_packs") {
        ShowScreen(SCREEN_TEXTURE_PACKS);
    }
    else if (buttonId == "btn_settings_mods") {
        ShowScreen(SCREEN_MOD_MANAGER);
    }
    else if (buttonId == "btn_cycle_theme") {
        extern void UIScreenManager_CycleTheme(Minecraft* mc);
        if (m_minecraft) UIScreenManager_CycleTheme(m_minecraft);
        ReloadCurrentScreen();
    }
    else if (buttonId == "btn_settings_back") {
        if (m_previousScreen == SCREEN_PAUSE)
            ShowScreen(SCREEN_PAUSE);
        else
            ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_audio_back") {
        extern void UIScreenManager_SaveOptions(Minecraft*);
        if (m_minecraft) UIScreenManager_SaveOptions(m_minecraft);
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_video_back") {
        extern void UIScreenManager_SaveOptions(Minecraft*);
        if (m_minecraft) UIScreenManager_SaveOptions(m_minecraft);
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_controls_back") {
        m_waitingForKey = false;
        m_keyBindIndex = -1;
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_tp_back") {
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_tp_enable") {
        SDL_Log("[UIScreenManager] Texture pack enable (not yet implemented)");
    }
    else if (buttonId == "btn_tp_disable") {
        SDL_Log("[UIScreenManager] Texture pack disable (not yet implemented)");
    }
    else if (buttonId == "btn_mod_back") {
        ShowScreen(SCREEN_SETTINGS);
    }
    else if (buttonId == "btn_mod_enable") {
        SDL_Log("[UIScreenManager] Mod enable (not yet implemented)");
    }
    else if (buttonId == "btn_mod_disable") {
        SDL_Log("[UIScreenManager] Mod disable (not yet implemented)");
    }

    else if (buttonId.size() > 8 && buttonId.substr(0, 8) == "keybind_") {
        int idx = atoi(buttonId.c_str() + 8);
        if (idx >= 0 && idx < 14) {
            m_waitingForKey = true;
            m_keyBindIndex = idx;
            if (auto* el = m_currentDoc->GetElementById(buttonId.c_str())) {
                el->SetInnerRML("&gt; Press a key... &lt;");
            }
        }
        return;
    }
    else if (buttonId == "btn_toggle_invert_y") {
        extern void UIScreenManager_ToggleInvertY(Minecraft*);
        if (m_minecraft) UIScreenManager_ToggleInvertY(m_minecraft);
        PopulateControlsSettings();
    }

    else if (buttonId == "btn_cycle_render_dist") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 4);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_graphics") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 10);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_gui_scale") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 12);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_particles") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 16);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_clouds") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 15);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_ao") {
        extern void UIScreenManager_CycleVideoOption(Minecraft*, int);
        if (m_minecraft) UIScreenManager_CycleVideoOption(m_minecraft, 11);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_window") {
        extern void UIScreenManager_CycleWindowMode(Minecraft*);
        if (m_minecraft) UIScreenManager_CycleWindowMode(m_minecraft);
        PopulateVideoSettings();
    }
    else if (buttonId == "btn_cycle_fps") {
        extern void UIScreenManager_CycleTargetFPS(Minecraft*);
        if (m_minecraft) UIScreenManager_CycleTargetFPS(m_minecraft);
        PopulateVideoSettings();
    }

    else if (buttonId == "btn_respawn") {
        SDL_Log("[UIScreenManager] Respawn clicked");
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
        if (m_minecraft) UIScreenManager_Respawn(m_minecraft);
    }
    else if (buttonId == "btn_death_quit") {
        SDL_Log("[UIScreenManager] Death → Quit to Title");
        HUDOverlay::Get().Shutdown();
        HideCurrent();
        if (m_minecraft) UIScreenManager_ReleaseLevel(m_minecraft);
        m_currentScreen = SCREEN_TITLE;
        ShowTitleScreen();
    }

    else if (buttonId == "btn_close_container" ||
             buttonId == "btn_close_furnace" || buttonId == "btn_close_chest" ||
             buttonId == "btn_close_enchant" || buttonId == "btn_close_brew" ||
             buttonId == "btn_close_anvil") {
        SDL_Log("[UIScreenManager] Closing container menu");
        FurnaceOverlay::Get().Hide();
        ChestOverlay::Get().Hide();
        EnchantingOverlay::Get().Hide();
        BrewingOverlay::Get().Hide();
        AnvilOverlay::Get().Hide();
        DispenserOverlay::Get().Hide();
        HopperOverlay::Get().Hide();
        BeaconOverlay::Get().Hide();
        HorseOverlay::Get().Hide();
        TradingOverlay::Get().Hide();
        GrindstoneOverlay::Get().Hide();
        StonecutterOverlay::Get().Hide();
        CartographyOverlay::Get().Hide();
        LoomOverlay::Get().Hide();
        SmithingOverlay::Get().Hide();
        FabricatorOverlay::Get().Hide();
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }

    else if (buttonId.substr(0, 11) == "enchant_opt") {
        SDL_Log("[UIScreenManager] Enchantment option: %s (not yet implemented)", buttonId.c_str());
    }

    else if (buttonId == "btn_join") {
        SDL_Log("[UIScreenManager] Join server (not yet implemented)");
    }
    else if (buttonId == "btn_join_back" || buttonId == "btn_host_back") {
        ShowScreen(SCREEN_WORLD_SELECT);
    }
    else if (buttonId == "btn_host_start") {
        SDL_Log("[UIScreenManager] Host game (not yet implemented)");
    }
    else if (buttonId == "btn_connect_cancel") {
        ShowScreen(SCREEN_WORLD_SELECT);
    }

    else if (buttonId == "btn_server_back") {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_server_join") {
        SDL_Log("[UIScreenManager] Server join (not yet implemented)");
    }
    else if (buttonId == "btn_server_direct") {
        SDL_Log("[UIScreenManager] Direct connect (not yet implemented)");
    }
    else if (buttonId == "btn_server_add") {
        SDL_Log("[UIScreenManager] Add server (not yet implemented)");
    }

    else if (buttonId == "btn_credits_back" || buttonId == "btn_howtoplay_back") {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_howtoplay_prev") {
        if (m_howToPlayPage > 0) m_howToPlayPage--;
        PopulateHowToPlayPage();
    }
    else if (buttonId == "btn_howtoplay_next") {
        if (m_howToPlayPage < 2) m_howToPlayPage++;
        PopulateHowToPlayPage();
    }
    else if (buttonId == "btn_eula_accept") {
        SDL_Log("[UIScreenManager] EULA accepted");
        extern void UIScreenManager_SetEulaAccepted(Minecraft*);
        if (m_minecraft) UIScreenManager_SetEulaAccepted(m_minecraft);
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_eula_decline") {
        SDL_Log("[UIScreenManager] EULA declined");
        if (m_minecraft) UIScreenManager_StopGame(m_minecraft);
    }

    else if (buttonId == "btn_chat_send") {
        if (m_currentDoc) {
            if (auto* inputEl = m_currentDoc->GetElementById("input_chat_message")) {
                Rml::String val = inputEl->GetAttribute<Rml::String>("value", "");
                if (!val.empty()) {
                    extern void UIScreenManager_SendChat(Minecraft* mc, const char* msg);
                    if (m_minecraft) UIScreenManager_SendChat(m_minecraft, val.c_str());
                    inputEl->SetAttribute("value", "");
                }
            }
        }
    }
    else if (buttonId == "btn_chat_close") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }

    else if (buttonId == "btn_close_creative") {
        CreativeOverlay::Get().Hide();
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }
    else if (buttonId == "btn_tab_blocks" || buttonId == "btn_tab_tools" ||
             buttonId == "btn_tab_food"   || buttonId == "btn_tab_misc"  ||
             buttonId == "btn_tab_search") {
        SDL_Log("[UIScreenManager] Creative tab: %s", buttonId.c_str());
    }

    else if (buttonId == "btn_book_done" || buttonId == "btn_book_sign" ||
             buttonId == "btn_book_close") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }
    else if (buttonId == "btn_book_prev" || buttonId == "btn_book_next") {
        SDL_Log("[UIScreenManager] Book page navigation: %s", buttonId.c_str());
    }

    else if (buttonId == "btn_sign_done") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }

    else if (buttonId == "btn_cmd_done" || buttonId == "btn_cmd_cancel") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }
    else if (buttonId == "btn_cmd_mode") {
        SDL_Log("[UIScreenManager] Command block mode cycle (not yet implemented)");
    }
    else if (buttonId == "btn_cmd_conditional") {
        SDL_Log("[UIScreenManager] Command block conditional toggle (not yet implemented)");
    }
    else if (buttonId == "btn_cmd_always") {
        SDL_Log("[UIScreenManager] Command block always-active toggle (not yet implemented)");
    }

    else if (buttonId == "btn_map_close") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }

    else if (buttonId == "btn_sleep" || buttonId == "btn_bed_leave") {
        SDL_Log("[UIScreenManager] Bed: %s", buttonId.c_str());
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }

    else if (buttonId == "btn_adv_close") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }
    else if (buttonId == "btn_adv_story"  || buttonId == "btn_adv_nether" ||
             buttonId == "btn_adv_end"    || buttonId == "btn_adv_adventure" ||
             buttonId == "btn_adv_husbandry") {
        SDL_Log("[UIScreenManager] Advancements tab: %s", buttonId.c_str());
    }

    else if (buttonId == "btn_stat_close") {
        HideCurrent();
        m_currentScreen = SCREEN_IN_GAME;
        HUDOverlay::Get().Show();
    }
    else if (buttonId == "btn_stat_general" || buttonId == "btn_stat_blocks" ||
             buttonId == "btn_stat_items"   || buttonId == "btn_stat_mobs") {
        SDL_Log("[UIScreenManager] Statistics tab: %s", buttonId.c_str());
    }

    else if (buttonId == "btn_friends_back") {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_friends_invite") {
        SDL_Log("[UIScreenManager] Friends invite (not yet implemented)");
    }

    else if (buttonId == "btn_skin_back") {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_skin_select" || buttonId == "btn_skin_import") {
        SDL_Log("[UIScreenManager] Dressing room: %s (not yet implemented)", buttonId.c_str());
    }

    else if (buttonId == "btn_hub_back") {
        ShowScreen(SCREEN_TITLE);
    }
    else if (buttonId == "btn_hub_mods"     || buttonId == "btn_hub_textures" ||
             buttonId == "btn_hub_skins"    || buttonId == "btn_hub_maps") {
        SDL_Log("[UIScreenManager] Community hub tab: %s", buttonId.c_str());
    }

    else if (buttonId == "btn_back") {
        if (m_currentScreen == SCREEN_WORLD_SELECT) {
            ShowScreen(SCREEN_TITLE);
        } else if (m_previousScreen != SCREEN_NONE) {
            ShowScreen(m_previousScreen);
        } else {
            ShowScreen(SCREEN_TITLE);
        }
    }

    else {
        SDL_Log("[UIScreenManager] Unknown button: %s", buttonId.c_str());
    }
}
