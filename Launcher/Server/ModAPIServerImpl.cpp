#include "..\Client\stdafx.h"
#include "..\ModAPI\ModAPI.h"
#include "..\ModAPI\ModTypes.h"
#include "..\Core\MinecraftServer.h"
#include "..\Core\Network\PlayerList.h"
#include "..\Core\Game\ServerPlayer.h"
#include "..\Core\Game\ServerPlayerGameMode.h"
#include "..\World\Level\LevelSettings.h"
#include "..\World\Level\Dimension.h"
#include "..\World\Level\Level.h"
#include "..\World\Network\ChatPacket.h"
#include "..\World\Player\FoodData.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <mutex>

struct RegisteredModCommand {
    std::string name;
    std::string description;
    ModCommandHandler handler;
    void* userData;
    int handle;
    bool active;
};

static std::vector<RegisteredModCommand> s_modCommands;
static std::mutex s_modCmdMtx;
static int s_nextModCmdHandle = 1;

// narrow wstring to malloc'd C string (caller must free); non-ASCII chars become '?'
static char* wstringToMallocStr(const std::wstring& ws) {
    size_t len = ws.size();
    char* buf = (char*)malloc(len + 1);
    if (!buf) return nullptr;
    for (size_t i = 0; i < len; i++) {
        wchar_t ch = ws[i];
        buf[i] = (ch < 128) ? (char)ch : '?';
    }
    buf[len] = 0;
    return buf;
}

static std::wstring narrowToWide(const char* s) {
    if (!s) return L"";
    std::wstring ws;
    while (*s) { ws += (wchar_t)(unsigned char)*s; s++; }
    return ws;
}

int ModAPI_GetPlayerList(ModPlayerInfo** outList, int* outCount) {
    PlayerList* pl = MinecraftServer::getPlayerList();
    if (!pl) {
        *outList = nullptr;
        *outCount = 0;
        return -1;
    }

    auto& players = pl->players;
    if (players.empty()) {
        *outList = nullptr;
        *outCount = 0;
        return 0;
    }

    int count = (int)players.size();
    *outCount = count;
    *outList = (ModPlayerInfo*)malloc(sizeof(ModPlayerInfo) * count);
    if (!*outList) { *outCount = 0; return -1; }

    for (int i = 0; i < count; i++) {
        auto& sp = players[i];
        ModPlayerInfo& info = (*outList)[i];

        info.handle = (ModPlayer)sp.get();

        // use displayName/name directly; getDisplayName/getName have linker deps we avoid
        std::wstring wname = sp->displayName;
        if (wname.empty()) wname = sp->name;
        info.name = wstringToMallocStr(wname);

        info.x = sp->x;
        info.y = sp->y;
        info.z = sp->z;
        info.dimension = (sp->level && sp->level->dimension) ? sp->level->dimension->id : sp->dimension;
        info.health = (float)sp->getHealth();

        GameType* gt = sp->gameMode ? sp->gameMode->getGameModeForPlayer() : nullptr;
        if (gt == GameType::CREATIVE)       info.gamemode = 1;
        else if (gt == GameType::ADVENTURE) info.gamemode = 2;
        else                                info.gamemode = 0; // SURVIVAL or unknown
    }

    return 0;
}

void ModAPI_FreePlayerList(ModPlayerInfo* list) {
    // name strings are individually malloc'd — caller must free them before this
    free(list);
}

void ModAPI_KickPlayer(ModPlayer player, const char* reason) {
    if (!player) return;
    ServerPlayer* sp = (ServerPlayer*)player;
    std::wstring wreason = narrowToWide(reason ? reason : "Kicked by server");
    sp->sendMessage(wreason);
    sp->disconnect();

    printf("[ModAPI] Kicked player: %s\n", reason ? reason : "");
}

void ModAPI_BanPlayer(const char* playerName, const char* reason) {
    // no persistent ban list (4J used XUID bans); kicks only
    if (!playerName) return;

    PlayerList* pl = MinecraftServer::getPlayerList();
    if (pl) {
        std::wstring wname = narrowToWide(playerName);
        auto sp = pl->getPlayer(wname);
        if (sp) {
            std::wstring wreason = narrowToWide(reason ? reason : "Banned");
            sp->sendMessage(wreason);
            sp->disconnect();
        }
    }

    printf("[ModAPI] Ban requested for '%s' (%s) — no persistent ban list yet\n",
           playerName, reason ? reason : "");
}

int ModAPI_RegisterCommand(const char* commandName, const char* description,
                            ModCommandHandler handler, void* userData) {
    std::lock_guard<std::mutex> lock(s_modCmdMtx);
    int h = s_nextModCmdHandle++;
    s_modCommands.push_back({commandName, description ? description : "", handler, userData, h, true});
    printf("[ModAPI] Registered command: /%s\n", commandName);
    return h;
}

void ModAPI_UnregisterCommand(int handle) {
    std::lock_guard<std::mutex> lock(s_modCmdMtx);
    for (auto& cmd : s_modCommands) {
        if (cmd.handle == handle) { cmd.active = false; break; }
    }
}

void ModAPI_BroadcastChat(const char* message) {
    if (!message) return;
    PlayerList* pl = MinecraftServer::getPlayerList();
    if (!pl) return;

    std::wstring wmsg = narrowToWide(message);
    auto packet = std::make_shared<ChatPacket>(wmsg, ChatPacket::e_ChatCustom, -1, L"");
    pl->broadcastAll(packet);
}

void ModAPI_TeleportPlayer(ModPlayer player, double x, double y, double z) {
    if (!player) return;
    ServerPlayer* sp = (ServerPlayer*)player;
    sp->moveTo(x, y, z, sp->yRot, sp->xRot);
}

void ModAPI_SetPlayerGamemode(ModPlayer player, int gamemode) {
    if (!player) return;
    ServerPlayer* sp = (ServerPlayer*)player;

    GameType* mode = GameType::SURVIVAL;
    switch (gamemode) {
        case 1: mode = GameType::CREATIVE;  break;
        case 2: mode = GameType::ADVENTURE; break;
        default: mode = GameType::SURVIVAL; break;
    }
    sp->setGameMode(mode);
}
