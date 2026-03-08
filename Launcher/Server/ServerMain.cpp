// Dedicated server implementation - wires MinecraftServer game logic
// Compiled with the full client stdafx to access MinecraftServer and all Core/World types
#include "..\Client\stdafx.h"
#include "ServerMain.h"
#include "Protocol\JavaListener.h"
#include "Protocol\BedrockHandler.h"
#include "..\Core\MinecraftServer.h"
#include "..\Core\Network\ServerConnection.h"
#include "..\Core\Network\PlayerList.h"
#include "..\Core\Network\RakNetTransport.h"
#include "..\Core\Network\NetworkAdapter.h"
#include "..\World\IO\Socket.h"
#include "..\World\IO\System.h"
#include "..\World\Math\AABB.h"
#include "..\World\Math\Vec3.h"
#include "..\World\Math\IntCache.h"
#include "..\World\IO\compression.h"
#include "..\World\FwdHeaders\net.minecraft.world.level.tile.h"
#include "..\World\Minecraft.World.h"
#include "..\World\Network\ChatPacket.h"
#include "..\World\Level\LevelData.h"
#include "..\World\Level\LevelSettings.h"
#include "..\Core\Game\ServerLevel.h"
#include "..\Core\Game\ServerPlayer.h"
#include "..\World\Math\Inventory.h"
#include "..\World\Items\ItemInstance.h"
#include "..\World\Player\FoodData.h"
#include "..\World\Level\Dimension.h"
#include "ServerData.h"
#include "ModDistributor.h"
#include "..\Client\ModLoader\ModLoader.h"

// defined in ServerStubs.cpp, read by GetGameHostOption
extern unsigned int g_serverDifficulty;
extern unsigned int g_serverGameType;

static unsigned int parseDifficulty(const wchar_t* s) {
    if (!s) return 2;
    std::wstring d(s);
    if (d == L"peaceful") return 0;
    if (d == L"easy") return 1;
    if (d == L"normal") return 2;
    if (d == L"hard") return 3;
    return 2;
}

static unsigned int parseGamemode(const wchar_t* s) {
    if (!s) return 0;
    std::wstring g(s);
    if (g == L"survival") return 0;
    if (g == L"creative") return 1;
    if (g == L"adventure") return 2;
    return 0;
}

static const char* difficultyName(unsigned int d) {
    switch (d) {
        case 0: return "peaceful"; case 1: return "easy";
        case 2: return "normal";   case 3: return "hard";
        default: return "unknown";
    }
}

static const char* gamemodeName(unsigned int g) {
    switch (g) {
        case 0: return "survival"; case 1: return "creative";
        case 2: return "adventure"; default: return "unknown";
    }
}

static RakNetTransport* s_transport = nullptr;
static NetworkAdapter* s_adapter = nullptr;
static JavaListener* s_javaListener = nullptr;
static BedrockHandler* s_bedrockHandler = nullptr;
static C4JThread* s_serverThread = nullptr;
static bool s_running = false;
static int64_t s_seed = 0;
static bool s_findSeed = true;

static std::wstring s_motd;
static int s_maxPlayers = 8;
static std::wstring s_worldName;
static std::wstring s_gamemode;

static int serverThreadFunc(void* param) {
    Entity::useSmallIds();
    IntCache::CreateNewThreadStorage();
    AABB::CreateNewThreadStorage();
    Vec3::CreateNewThreadStorage();
    Compression::UseDefaultThreadStorage();
    Tile::CreateNewThreadStorage();

    NetworkGameInitData initData;
    initData.seed = s_seed;
    initData.findSeed = s_findSeed;
    initData.xzSize = LEVEL_LEGACY_WIDTH;
    initData.hellScale = HELL_LEVEL_LEGACY_SCALE;

    if (s_findSeed)
        printf("[Server] Starting world generation (random seed)...\n");
    else
        printf("[Server] Starting world generation (seed: %lld)...\n", (long long)s_seed);
    __try {
        MinecraftServer::main(0, &initData);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("[Server] CRASH in server thread! Exception code: 0x%08X\n", GetExceptionCode());
    }
    printf("[Server] Server thread exited.\n");

    Tile::ReleaseThreadStorage();
    IntCache::ReleaseThreadStorage();
    AABB::ReleaseThreadStorage();
    Vec3::ReleaseThreadStorage();

    s_running = false;
    return 0;
}

static std::vector<std::string> splitArgs(const char* line) {
    std::vector<std::string> args;
    std::string current;
    while (*line) {
        if (*line == ' ') {
            if (!current.empty()) { args.push_back(current); current.clear(); }
        } else {
            current += *line;
        }
        line++;
    }
    if (!current.empty()) args.push_back(current);
    return args;
}

static std::string restOfLine(const char* line, int skipTokens) {
    while (*line == ' ') line++;
    for (int i = 0; i < skipTokens && *line; i++) {
        while (*line && *line != ' ') line++;
        while (*line == ' ') line++;
    }
    return std::string(line);
}

static std::shared_ptr<ServerPlayer> findPlayer(MinecraftServer* srv, const std::string& name) {
    if (!srv) return nullptr;
    PlayerList* pl = srv->getPlayers();
    if (!pl) return nullptr;
    std::wstring wname(name.begin(), name.end());
    return pl->getPlayer(wname);
}

static void storeBack(const std::string& name, std::shared_ptr<ServerPlayer> player) {
    Position pos;
    pos.x = player->x; pos.y = player->y; pos.z = player->z;
    pos.yaw = player->yRot; pos.pitch = player->xRot;
    pos.dimension = player->level && player->level->dimension ? player->level->dimension->id : 0;
    ServerData::instance().setBack(name, pos);
}

namespace DedicatedServer {

bool init(const ServerInitParams& params) {
    s_seed = params.seed;
    s_findSeed = params.findSeed;

    g_serverDifficulty = parseDifficulty(params.difficulty);
    g_serverGameType = parseGamemode(params.gamemode);

    s_motd = params.motd ? std::wstring(params.motd) : L"Ourcraft Server";
    s_maxPlayers = params.maxPlayers;
    s_worldName = params.worldName ? std::wstring(params.worldName) : L"world";
    s_gamemode = params.gamemode ? std::wstring(params.gamemode) : L"survival";

    Entity::useSmallIds();
    IntCache::CreateNewThreadStorage();
    AABB::CreateNewThreadStorage();
    Vec3::CreateNewThreadStorage();
    Compression::UseDefaultThreadStorage();
    Tile::CreateNewThreadStorage();

    ServerData::instance().load(".");

    ModLoader::Get().Initialize("mods");
    ModLoader::Get().LoadAllMods();
    ModLoader::Get().InitializeAllMods();
    printf("[Server] Loaded %d mod(s)\n", ModLoader::Get().GetModCount());

    ModDistributor::instance().init(".", "mod-keys/private.key");

    MinecraftWorld_RunStaticCtors();

    s_transport = new RakNetTransport();
    if (!s_transport->listen(params.bedrockPort, params.maxPlayers)) {
        printf("[Bedrock] ERROR: RakNet failed to listen on port %u\n", params.bedrockPort);
        delete s_transport;
        s_transport = nullptr;
        return false;
    }

    s_adapter = new NetworkAdapter(s_transport);
    Socket::SetNetworkAdapter(s_adapter);

    s_bedrockHandler = new BedrockHandler();
    s_bedrockHandler->init(s_transport, params.bedrockPort);
    s_bedrockHandler->setOnlineMode(params.onlineMode);
    s_bedrockHandler->updateMotd(s_motd, 0, s_maxPlayers, s_worldName, s_gamemode);

    printf("[Bedrock] Listening on UDP port %u\n", params.bedrockPort);

    s_javaListener = new JavaListener();
    s_javaListener->setMotd(s_motd);
    s_javaListener->setMaxPlayers(params.maxPlayers);
    s_javaListener->setOnlineMode(params.onlineMode);

    if (!s_javaListener->start(params.javaPort, params.maxPlayers)) {
        printf("[Java] WARNING: Failed to listen on TCP port %u (Java clients won't connect)\n", params.javaPort);
        delete s_javaListener;
        s_javaListener = nullptr;
        // non-fatal: Bedrock still works
    } else {
        printf("[Java] Listening on TCP port %u\n", params.javaPort);
    }

    s_running = true;

    s_serverThread = new C4JThread(serverThreadFunc, nullptr, "DedicatedServer", 512 * 1024);
    s_serverThread->Run();

    return true;
}

void tick() {
    if (!s_running) return;

    int totalPlayers = getPlayerCount();

    if (s_transport) {
        s_adapter->tick();

        ConnectionId connId;
        while (s_transport->pollNewConnection(connId)) {
            printf("[Bedrock] Player connected (id=%llu, total=%d)\n",
                   connId.id, s_transport->getConnectionCount());
        }
        while (s_transport->pollDisconnection(connId)) {
            printf("[Bedrock] Player disconnected (id=%llu, total=%d)\n",
                   connId.id, s_transport->getConnectionCount());
        }

        IncomingPacket* pkt = nullptr;
        while ((pkt = s_transport->receive()) != nullptr) {
            s_transport->deallocatePacket(pkt);
        }

        if (s_bedrockHandler) {
            s_bedrockHandler->updateMotd(s_motd, totalPlayers, s_maxPlayers, s_worldName, s_gamemode);
            s_bedrockHandler->tick();
        }
    }

    ModLoader::Get().TickAllMods(0.05f);

    if (s_javaListener) {
        s_javaListener->setOnlinePlayers(totalPlayers);
        s_javaListener->tick();
    }
}

void shutdown() {
    s_running = false;

    if (s_javaListener) {
        s_javaListener->disconnectAll("Server shutting down");
        s_javaListener->stop();
        delete s_javaListener;
        s_javaListener = nullptr;
    }

    if (s_bedrockHandler) {
        delete s_bedrockHandler;
        s_bedrockHandler = nullptr;
    }

    ModLoader::Get().ShutdownAllMods();

    ServerData::instance().save(".");

    MinecraftServer* srv = MinecraftServer::getInstance();
    if (srv) {
        printf("[Server] Halting game server...\n");
        srv->halt();
    }

    if (s_serverThread) {
        s_serverThread->WaitForCompletion(10000);
        delete s_serverThread;
        s_serverThread = nullptr;
    }

    if (s_transport) {
        Socket::SetNetworkAdapter(nullptr);
        s_transport->stop();
        delete s_adapter;
        delete s_transport;
        s_adapter = nullptr;
        s_transport = nullptr;
    }
}

bool isRunning() {
    return s_running;
}

int getPlayerCount() {
    int count = 0;
    if (s_transport) count += s_transport->getConnectionCount();
    if (s_javaListener) count += s_javaListener->getConnectionCount();
    return count;
}

int getBedrockPlayerCount() {
    return s_transport ? s_transport->getConnectionCount() : 0;
}

int getJavaPlayerCount() {
    return s_javaListener ? s_javaListener->getConnectionCount() : 0;
}

void executeCommand(const char* line) {
    auto args = splitArgs(line);
    if (args.empty()) return;

    const std::string& cmd = args[0];
    MinecraftServer* srv = MinecraftServer::getInstance();

    if (cmd == "list") {
        int bedrock = getBedrockPlayerCount();
        int java = getJavaPlayerCount();
        printf("[Server] Players online: %d (Bedrock: %d, Java: %d)\n", bedrock + java, bedrock, java);

        if (s_javaListener) {
            auto names = s_javaListener->getPlayerNames();
            if (!names.empty()) {
                printf("[Java] ");
                for (size_t i = 0; i < names.size(); i++) {
                    if (i > 0) printf(", ");
                    printf("%s", names[i].c_str());
                }
                printf("\n");
            }
        }

        if (srv) {
            PlayerList* pl = srv->getPlayers();
            if (pl && pl->getPlayerCount() > 0) {
                std::wstring names = pl->getPlayerNames();
                printf("[Game] %S\n", names.c_str());
            }
        }
    }
    else if (cmd == "say") {
        std::string msg = restOfLine(line, 1);
        if (msg.empty()) { printf("Usage: say <message>\n"); return; }

        printf("[Server] [Server] %s\n", msg.c_str());

        if (s_javaListener) {
            s_javaListener->broadcastChat("[Server] " + msg);
        }

        if (srv) {
            PlayerList* pl = srv->getPlayers();
            if (pl) {
                std::wstring wmsg(msg.begin(), msg.end());
                auto pkt = std::make_shared<ChatPacket>(L"[Server] " + wmsg);
                pl->broadcastAll(std::static_pointer_cast<Packet>(pkt));
            }
        }
    }
    else if (cmd == "kick") {
        if (args.size() < 2) { printf("Usage: kick <player> [reason]\n"); return; }
        std::string target = args[1];
        std::string reason = args.size() > 2 ? restOfLine(line, 2) : "Kicked by server";

        bool kicked = false;

        if (s_javaListener && s_javaListener->kickPlayer(target, reason)) {
            printf("[Server] Kicked Java player: %s\n", target.c_str());
            kicked = true;
        }

        if (!kicked && srv) {
            PlayerList* pl = srv->getPlayers();
            if (pl) {
                std::wstring wname(target.begin(), target.end());
                auto player = pl->getPlayer(wname);
                if (player) {
                    pl->remove(player);
                    printf("[Server] Kicked game player: %s\n", target.c_str());
                    kicked = true;
                }
            }
        }

        if (!kicked) printf("[Server] Player '%s' not found\n", target.c_str());
    }
    else if (cmd == "seed") {
        if (srv) {
            ServerLevel* level = srv->getLevel(0);
            if (level) {
                LevelData* data = level->getLevelData();
                if (data) {
                    printf("[Server] Seed: %lld\n", (long long)data->getSeed());
                    return;
                }
            }
        }
        if (s_seed != 0)
            printf("[Server] Seed: %lld (config)\n", (long long)s_seed);
        else
            printf("[Server] Seed: (random, world not loaded yet)\n");
    }
    else if (cmd == "difficulty") {
        if (args.size() < 2) {
            printf("[Server] Difficulty: %s (%u)\n", difficultyName(g_serverDifficulty), g_serverDifficulty);
            return;
        }
        std::wstring wval(args[1].begin(), args[1].end());
        unsigned int d = parseDifficulty(wval.c_str());
        g_serverDifficulty = d;

        if (srv) {
            for (int i = 0; i < 3; i++) {
                ServerLevel* level = srv->getLevel(i);
                if (level) level->difficulty = d;
            }
        }
        printf("[Server] Difficulty set to: %s\n", difficultyName(d));
    }
    else if (cmd == "gamemode") {
        if (args.size() < 2) {
            printf("[Server] Default gamemode: %s (%u)\n", gamemodeName(g_serverGameType), g_serverGameType);
            return;
        }
        std::wstring wval(args[1].begin(), args[1].end());
        unsigned int g = parseGamemode(wval.c_str());
        g_serverGameType = g;

        if (srv) {
            PlayerList* pl = srv->getPlayers();
            if (pl) {
                GameType* gt = nullptr;
                switch (g) {
                    case 0: gt = GameType::SURVIVAL; break;
                    case 1: gt = GameType::CREATIVE; break;
                    case 2: gt = GameType::ADVENTURE; break;
                }
                if (gt) pl->setOverrideGameMode(gt);
            }
        }
        printf("[Server] Default gamemode set to: %s\n", gamemodeName(g));
    }
    else if (cmd == "time") {
        if (args.size() < 2) { printf("Usage: time <set|query> [value]\n"); return; }

        if (args[1] == "query") {
            if (srv) {
                ServerLevel* level = srv->getLevel(0);
                if (level) {
                    LevelData* data = level->getLevelData();
                    if (data) {
                        printf("[Server] Time: %lld (day %lld)\n",
                               (long long)data->getTime(),
                               (long long)(data->getTime() / 24000));
                        return;
                    }
                }
            }
            printf("[Server] World not loaded\n");
        } else if (args[1] == "set") {
            if (args.size() < 3) { printf("Usage: time set <value|day|night|noon|midnight>\n"); return; }

            __int64 t = 0;
            if (args[2] == "day") t = 1000;
            else if (args[2] == "night") t = 13000;
            else if (args[2] == "noon") t = 6000;
            else if (args[2] == "midnight") t = 18000;
            else {
                try { t = std::stoll(args[2]); }
                catch (...) { printf("[Server] Invalid time value\n"); return; }
            }

            if (srv) {
                for (int i = 0; i < 3; i++) {
                    ServerLevel* level = srv->getLevel(i);
                    if (level) {
                        LevelData* data = level->getLevelData();
                        if (data) data->setTime(t);
                    }
                }
                printf("[Server] Time set to %lld\n", (long long)t);
            } else {
                printf("[Server] World not loaded\n");
            }
        } else {
            printf("Usage: time <set|query> [value]\n");
        }
    }
    else if (cmd == "weather") {
        if (args.size() < 2) { printf("Usage: weather <clear|rain|thunder>\n"); return; }

        if (!srv) { printf("[Server] World not loaded\n"); return; }
        ServerLevel* level = srv->getLevel(0);
        if (!level) { printf("[Server] World not loaded\n"); return; }
        LevelData* data = level->getLevelData();
        if (!data) { printf("[Server] World not loaded\n"); return; }

        if (args[1] == "clear") {
            level->setRainLevel(0.0f);
            data->setRainTime(0);
            data->setThunderTime(0);
            printf("[Server] Weather set to clear\n");
        } else if (args[1] == "rain") {
            level->setRainLevel(1.0f);
            data->setRainTime(24000);
            data->setThunderTime(0);
            printf("[Server] Weather set to rain\n");
        } else if (args[1] == "thunder") {
            level->setRainLevel(1.0f);
            data->setRainTime(24000);
            data->setThunderTime(24000);
            printf("[Server] Weather set to thunder\n");
        } else {
            printf("Usage: weather <clear|rain|thunder>\n");
        }
    }
    else if (cmd == "tp") {
        if (args.size() < 5) { printf("Usage: tp <player> <x> <y> <z>\n"); return; }
        if (!srv) { printf("[Server] World not loaded\n"); return; }

        PlayerList* pl = srv->getPlayers();
        if (!pl) { printf("[Server] World not loaded\n"); return; }

        std::wstring wname(args[1].begin(), args[1].end());
        auto player = pl->getPlayer(wname);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }

        try {
            double x = std::stod(args[2]);
            double y = std::stod(args[3]);
            double z = std::stod(args[4]);
            player->moveTo(x, y, z, player->yRot, player->xRot);
            printf("[Server] Teleported %s to %.1f %.1f %.1f\n", args[1].c_str(), x, y, z);
        } catch (...) {
            printf("[Server] Invalid coordinates\n");
        }
    }
    else if (cmd == "give") {
        if (args.size() < 3) { printf("Usage: give <player> <itemId> [count]\n"); return; }
        if (!srv) { printf("[Server] World not loaded\n"); return; }

        PlayerList* pl = srv->getPlayers();
        if (!pl) { printf("[Server] World not loaded\n"); return; }

        std::wstring wname(args[1].begin(), args[1].end());
        auto player = pl->getPlayer(wname);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }

        try {
            int itemId = std::stoi(args[2]);
            int count = args.size() > 3 ? std::stoi(args[3]) : 1;
            if (count < 1) count = 1;
            if (count > 64) count = 64;

            auto item = std::make_shared<ItemInstance>(itemId, count, 0);
            if (player->inventory) {
                player->inventory->add(item);
                printf("[Server] Gave %d x %d to %s\n", count, itemId, args[1].c_str());
            }
        } catch (...) {
            printf("[Server] Invalid item ID or count\n");
        }
    }
    else if (cmd == "save-all") {
        if (!srv) { printf("[Server] World not loaded\n"); return; }
        printf("[Server] Saving...\n");
        PlayerList* pl = srv->getPlayers();
        if (pl) pl->saveAll(nullptr);
        for (int i = 0; i < 3; i++) {
            ServerLevel* level = srv->getLevel(i);
            if (level) level->save(true, nullptr);
        }
        ServerData::instance().save(".");
        printf("[Server] Save complete\n");
    }
    else if (cmd == "heal") {
        if (args.size() < 2) { printf("Usage: heal <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        player->setHealth(Player::MAX_HEALTH);
        FoodData* food = player->getFoodData();
        if (food) { food->setFoodLevel(20); }
        printf("[Server] Healed %s\n", args[1].c_str());
    }
    else if (cmd == "feed") {
        if (args.size() < 2) { printf("Usage: feed <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        FoodData* food = player->getFoodData();
        if (food) { food->setFoodLevel(20); }
        printf("[Server] Fed %s\n", args[1].c_str());
    }
    else if (cmd == "kill") {
        if (args.size() < 2) { printf("Usage: kill <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        player->setHealth(0);
        printf("[Server] Killed %s\n", args[1].c_str());
    }
    else if (cmd == "clear") {
        if (args.size() < 2) { printf("Usage: clear <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        if (player->inventory) {
            player->inventory->clearInventory();
            printf("[Server] Cleared inventory of %s\n", args[1].c_str());
        }
    }
    else if (cmd == "fly") {
        if (args.size() < 2) { printf("Usage: fly <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        player->abilities.mayfly = !player->abilities.mayfly;
        if (!player->abilities.mayfly) player->abilities.flying = false;
        printf("[Server] Flight %s for %s\n",
               player->abilities.mayfly ? "enabled" : "disabled", args[1].c_str());
    }
    else if (cmd == "god") {
        if (args.size() < 2) { printf("Usage: god <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        player->abilities.invulnerable = !player->abilities.invulnerable;
        printf("[Server] God mode %s for %s\n",
               player->abilities.invulnerable ? "enabled" : "disabled", args[1].c_str());
    }
    else if (cmd == "sethome") {
        if (args.size() < 2) { printf("Usage: sethome <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        Position pos;
        pos.x = player->x; pos.y = player->y; pos.z = player->z;
        pos.yaw = player->yRot; pos.pitch = player->xRot;
        pos.dimension = player->level && player->level->dimension ? player->level->dimension->id : 0;
        ServerData::instance().setHome(args[1], pos);
        printf("[Server] Home set for %s at %.1f %.1f %.1f\n", args[1].c_str(), pos.x, pos.y, pos.z);
    }
    else if (cmd == "home") {
        if (args.size() < 2) { printf("Usage: home <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        Position pos;
        if (!ServerData::instance().getHome(args[1], pos)) {
            printf("[Server] No home set for %s\n", args[1].c_str());
            return;
        }
        storeBack(args[1], player);
        player->moveTo(pos.x, pos.y, pos.z, pos.yaw, pos.pitch);
        printf("[Server] Teleported %s to home (%.1f %.1f %.1f)\n", args[1].c_str(), pos.x, pos.y, pos.z);
    }
    else if (cmd == "delhome") {
        if (args.size() < 2) { printf("Usage: delhome <player>\n"); return; }
        if (ServerData::instance().delHome(args[1]))
            printf("[Server] Home deleted for %s\n", args[1].c_str());
        else
            printf("[Server] No home set for %s\n", args[1].c_str());
    }
    else if (cmd == "spawn") {
        if (args.size() < 2) { printf("Usage: spawn <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        if (!srv) { printf("[Server] World not loaded\n"); return; }
        ServerLevel* level = srv->getLevel(0);
        if (!level || !level->getLevelData()) { printf("[Server] World not loaded\n"); return; }
        LevelData* data = level->getLevelData();
        storeBack(args[1], player);
        player->moveTo((double)data->getXSpawn() + 0.5, (double)data->getYSpawn(),
                        (double)data->getZSpawn() + 0.5, player->yRot, player->xRot);
        printf("[Server] Teleported %s to spawn\n", args[1].c_str());
    }
    else if (cmd == "setspawn") {
        if (args.size() < 4) { printf("Usage: setspawn <x> <y> <z>\n"); return; }
        if (!srv) { printf("[Server] World not loaded\n"); return; }
        ServerLevel* level = srv->getLevel(0);
        if (!level || !level->getLevelData()) { printf("[Server] World not loaded\n"); return; }
        try {
            int x = std::stoi(args[1]);
            int y = std::stoi(args[2]);
            int z = std::stoi(args[3]);
            level->getLevelData()->setSpawn(x, y, z);
            printf("[Server] Spawn set to %d %d %d\n", x, y, z);
        } catch (...) {
            printf("[Server] Invalid coordinates\n");
        }
    }
    else if (cmd == "setwarp") {
        if (args.size() < 5) { printf("Usage: setwarp <name> <x> <y> <z> [dim]\n"); return; }
        try {
            Position pos;
            pos.x = std::stod(args[2]); pos.y = std::stod(args[3]); pos.z = std::stod(args[4]);
            pos.yaw = 0; pos.pitch = 0;
            pos.dimension = args.size() > 5 ? std::stoi(args[5]) : 0;
            ServerData::instance().setWarp(args[1], pos);
            printf("[Server] Warp '%s' set at %.1f %.1f %.1f (dim %d)\n",
                   args[1].c_str(), pos.x, pos.y, pos.z, pos.dimension);
        } catch (...) {
            printf("[Server] Invalid coordinates\n");
        }
    }
    else if (cmd == "warp") {
        if (args.size() < 3) { printf("Usage: warp <name> <player>\n"); return; }
        Position pos;
        if (!ServerData::instance().getWarp(args[1], pos)) {
            printf("[Server] Warp '%s' not found\n", args[1].c_str());
            return;
        }
        auto player = findPlayer(srv, args[2]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[2].c_str()); return; }
        storeBack(args[2], player);
        player->moveTo(pos.x, pos.y, pos.z, pos.yaw, pos.pitch);
        printf("[Server] Teleported %s to warp '%s'\n", args[2].c_str(), args[1].c_str());
    }
    else if (cmd == "delwarp") {
        if (args.size() < 2) { printf("Usage: delwarp <name>\n"); return; }
        if (ServerData::instance().delWarp(args[1]))
            printf("[Server] Warp '%s' deleted\n", args[1].c_str());
        else
            printf("[Server] Warp '%s' not found\n", args[1].c_str());
    }
    else if (cmd == "warps") {
        auto warps = ServerData::instance().listWarps();
        if (warps.empty()) { printf("[Server] No warps set\n"); return; }
        printf("[Server] Warps (%d): ", (int)warps.size());
        for (size_t i = 0; i < warps.size(); i++) {
            if (i > 0) printf(", ");
            printf("%s", warps[i].c_str());
        }
        printf("\n");
    }
    else if (cmd == "back") {
        if (args.size() < 2) { printf("Usage: back <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        Position pos;
        if (!ServerData::instance().getBack(args[1], pos)) {
            printf("[Server] No back position for %s\n", args[1].c_str());
            return;
        }
        storeBack(args[1], player);
        player->moveTo(pos.x, pos.y, pos.z, pos.yaw, pos.pitch);
        printf("[Server] Teleported %s back to %.1f %.1f %.1f\n", args[1].c_str(), pos.x, pos.y, pos.z);
    }
    else if (cmd == "kit") {
        if (args.size() < 2) {
            auto kits = ServerData::instance().listKits();
            printf("[Server] Kits: ");
            for (size_t i = 0; i < kits.size(); i++) {
                if (i > 0) printf(", ");
                printf("%s", kits[i].c_str());
            }
            printf("\n");
            return;
        }
        if (args.size() < 3) { printf("Usage: kit <name> <player>\n"); return; }

        const Kit* kit = ServerData::instance().getKit(args[1]);
        if (!kit) { printf("[Server] Kit '%s' not found (type 'kit' for list)\n", args[1].c_str()); return; }

        auto player = findPlayer(srv, args[2]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[2].c_str()); return; }
        if (!player->inventory) { printf("[Server] Player has no inventory\n"); return; }

        int given = 0;
        for (const auto& ki : kit->items) {
            auto item = std::make_shared<ItemInstance>(ki.id, ki.count, ki.damage);
            if (player->inventory->add(item)) given++;
        }
        printf("[Server] Gave kit '%s' to %s (%d items)\n", args[1].c_str(), args[2].c_str(), given);
    }
    else if (cmd == "msg" || cmd == "tell" || cmd == "whisper") {
        if (args.size() < 3) { printf("Usage: msg <player> <message>\n"); return; }
        std::string msg = restOfLine(line, 2);

        if (srv) {
            PlayerList* pl = srv->getPlayers();
            if (pl) {
                std::wstring wname(args[1].begin(), args[1].end());
                std::wstring wmsg(msg.begin(), msg.end());
                pl->sendMessage(wname, L"[Server -> You] " + wmsg);
                printf("[Server -> %s] %s\n", args[1].c_str(), msg.c_str());
                return;
            }
        }

        if (s_javaListener) {
            printf("[Server] Cannot send private message (player not in game world)\n");
        }
    }
    else if (cmd == "motd") {
        if (args.size() < 2) {
            std::string narrow(s_motd.begin(), s_motd.end());
            printf("[Server] MOTD: %s\n", narrow.c_str());
            return;
        }
        std::string newMotd = restOfLine(line, 1);
        s_motd = std::wstring(newMotd.begin(), newMotd.end());
        if (s_javaListener) s_javaListener->setMotd(s_motd);
        printf("[Server] MOTD set to: %s\n", newMotd.c_str());
    }
    else if (cmd == "whois") {
        if (args.size() < 2) { printf("Usage: whois <player>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        printf("[Server] === Player Info: %s ===\n", args[1].c_str());
        printf("  Health: %d/%d\n", player->getHealth(), Player::MAX_HEALTH);
        FoodData* food = player->getFoodData();
        if (food) printf("  Food: %d/20\n", food->getFoodLevel());
        printf("  Position: %.1f %.1f %.1f\n", player->x, player->y, player->z);
        printf("  Dimension: %d\n", (player->level && player->level->dimension) ? player->level->dimension->id : -1);
        printf("  Flight: %s | God: %s\n",
               player->abilities.mayfly ? "on" : "off",
               player->abilities.invulnerable ? "on" : "off");
        Position home;
        if (ServerData::instance().getHome(args[1], home))
            printf("  Home: %.1f %.1f %.1f\n", home.x, home.y, home.z);
        else
            printf("  Home: not set\n");
    }
    else if (cmd == "tpto") {
        if (args.size() < 3) { printf("Usage: tpto <player> <target>\n"); return; }
        auto player = findPlayer(srv, args[1]);
        auto target = findPlayer(srv, args[2]);
        if (!player) { printf("[Server] Player '%s' not found\n", args[1].c_str()); return; }
        if (!target) { printf("[Server] Target '%s' not found\n", args[2].c_str()); return; }
        storeBack(args[1], player);
        player->moveTo(target->x, target->y, target->z, target->yRot, target->xRot);
        printf("[Server] Teleported %s to %s\n", args[1].c_str(), args[2].c_str());
    }
    else if (cmd == "mod-keygen") {
        ModDistributor::instance().generateKeyPair("mod-keys");
    }
    else if (cmd == "mod-sign") {
        if (args.size() < 2) { printf("Usage: mod-sign <path-to-dll>\n"); }
        else { ModDistributor::instance().signMod(args[1].c_str()); }
    }
    else if (cmd == "mod-list") {
        auto& dist = ModDistributor::instance();
        printf("Mods loaded: %zu\n", dist.getMods().size());
        for (const auto& m : dist.getMods())
            printf("  - %s v%s (%u bytes)\n", m.modId.c_str(), m.version.c_str(), m.fileSize);
        printf("Resource packs: %zu\n", dist.getPacks().size());
        for (const auto& p : dist.getPacks())
            printf("  - %s v%s (%u bytes)\n", p.name.c_str(), p.version.c_str(), p.fileSize);
    }
    else if (cmd == "stop" || cmd == "quit" || cmd == "exit") {
        printf("[Server] Shutting down...\n");
        s_running = false;
    }
    else if (cmd == "status") {
        int bedrock = getBedrockPlayerCount();
        int java = getJavaPlayerCount();
        printf("[Server] Running: %s | Players: %d (Bedrock: %d, Java: %d)\n",
               isRunning() ? "yes" : "no", bedrock + java, bedrock, java);
        printf("[Server] Difficulty: %s | Gamemode: %s\n",
               difficultyName(g_serverDifficulty), gamemodeName(g_serverGameType));
        if (srv) {
            ServerLevel* level = srv->getLevel(0);
            if (level && level->getLevelData()) {
                LevelData* data = level->getLevelData();
                printf("[Server] Time: %lld (day %lld) | Weather: %s\n",
                       (long long)data->getTime(),
                       (long long)(data->getTime() / 24000),
                       level->isThundering() ? "thunder" : level->isRaining() ? "rain" : "clear");
            }
        }
    }
    else if (cmd == "help" || cmd == "?") {
        if (args.size() > 1 && args[1] == "2") {
            printf("=== Ourcraft Essentials Commands (page 2) ===\n");
            printf("  heal <player>          - Heal and feed player\n");
            printf("  feed <player>          - Feed player\n");
            printf("  kill <player>          - Kill player\n");
            printf("  clear <player>         - Clear player inventory\n");
            printf("  fly <player>           - Toggle flight\n");
            printf("  god <player>           - Toggle god mode\n");
            printf("  sethome <player>       - Set player's home\n");
            printf("  home <player>          - Teleport to home\n");
            printf("  delhome <player>       - Delete home\n");
            printf("  spawn <player>         - Teleport to spawn\n");
            printf("  setspawn <x> <y> <z>   - Set world spawn\n");
            printf("  setwarp <name> <x> <y> <z> - Create warp\n");
            printf("  warp <name> <player>   - Teleport to warp\n");
            printf("  delwarp <name>         - Delete warp\n");
            printf("  warps                  - List all warps\n");
            printf("  back <player>          - Return to last position\n");
            printf("  kit [name] <player>    - Give item kit\n");
            printf("  tpto <player> <target> - Teleport player to player\n");
            printf("  msg <player> <msg>     - Private message\n");
            printf("  motd [new motd]        - Show/set MOTD\n");
            printf("  whois <player>         - Player info\n");
        } else {
            printf("=== Ourcraft Server Commands (page 1) ===\n");
            printf("  help           - Show this help\n");
            printf("  help 2         - Essentials commands\n");
            printf("  status         - Server status, time, weather\n");
            printf("  list           - List connected players\n");
            printf("  say <msg>      - Broadcast message to all\n");
            printf("  kick <player> [reason] - Kick a player\n");
            printf("  seed           - Show world seed\n");
            printf("  difficulty [level]     - Get/set difficulty\n");
            printf("  gamemode [mode]        - Get/set default gamemode\n");
            printf("  time set <val>         - Set time (day/night/noon/ticks)\n");
            printf("  time query             - Show current time\n");
            printf("  weather <clear|rain|thunder> - Set weather\n");
            printf("  tp <player> <x> <y> <z>     - Teleport to coords\n");
            printf("  give <player> <id> [count]  - Give item\n");
            printf("  save-all       - Force save all chunks\n");
            printf("  stop           - Stop the server\n");
        }
    }
    else {
        printf("[Server] Unknown command: %s (type 'help')\n", cmd.c_str());
    }
}

} // namespace DedicatedServer
