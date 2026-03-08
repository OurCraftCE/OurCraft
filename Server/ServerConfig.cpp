#include "ServerConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale>
#include <codecvt>

static std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    return std::wstring(s.begin(), s.end());
}

static std::string toNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    return std::string(w.begin(), w.end());
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

ServerConfig ServerConfig::load(const char* path) {
    ServerConfig cfg;
    std::ifstream file(path);
    if (!file.is_open()) {
        cfg.save(path); // write defaults on first run
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "port" || key == "bedrock-port")
            cfg.bedrockPort = static_cast<uint16_t>(std::stoi(val));
        else if (key == "java-port")    cfg.javaPort = static_cast<uint16_t>(std::stoi(val));
        else if (key == "max-players")  cfg.maxPlayers = std::stoi(val);
        else if (key == "motd")         cfg.motd = toWide(val);
        else if (key == "world-name")   cfg.worldName = toWide(val);
        else if (key == "view-distance")cfg.viewDistance = std::stoi(val);
        else if (key == "online-mode")  cfg.onlineMode = (val == "true" || val == "1");
        else if (key == "gamemode")     cfg.gamemode = toWide(val);
        else if (key == "difficulty")   cfg.difficulty = toWide(val);
        else if (key == "seed") {
            if (val.empty() || val == "0") {
                cfg.seed = 0;
                cfg.findSeed = true;
            } else {
                try { cfg.seed = std::stoll(val); cfg.findSeed = false; }
                catch (...) { cfg.seed = 0; cfg.findSeed = true; }
            }
        }
        else if (key == "mod-signing-key")        cfg.modSigningKey = toWide(val);
        else if (key == "mod-public-key")          cfg.modPublicKey = toWide(val);
        else if (key == "enable-mods")             cfg.enableMods = (val == "true" || val == "1");
        else if (key == "force-server-packs")      cfg.forceServerPacks = (val == "true" || val == "1");
        else if (key == "resource-pack-max-size")  { try { cfg.resourcePackMaxSize = std::stoi(val); } catch(...) {} }
        else if (key == "transfer-chunk-size")     { try { cfg.transferChunkSize = std::stoi(val); } catch(...) {} }
        else if (key == "max-concurrent-transfers"){ try { cfg.maxConcurrentTransfers = std::stoi(val); } catch(...) {} }
    }

    return cfg;
}

void ServerConfig::save(const char* path) const {
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "# Ourcraft Dedicated Server Configuration\n";
    file << "bedrock-port=" << bedrockPort << "\n";
    file << "java-port=" << javaPort << "\n";
    file << "max-players=" << maxPlayers << "\n";
    file << "motd=" << toNarrow(motd) << "\n";
    file << "world-name=" << toNarrow(worldName) << "\n";
    file << "view-distance=" << viewDistance << "\n";
    file << "online-mode=" << (onlineMode ? "true" : "false") << "\n";
    file << "gamemode=" << toNarrow(gamemode) << "\n";
    file << "difficulty=" << toNarrow(difficulty) << "\n";
    if (seed != 0)
        file << "seed=" << seed << "\n";
    else
        file << "seed=\n";
    file << "\n# Mod System\n";
    file << "enable-mods=" << (enableMods ? "true" : "false") << "\n";
    file << "mod-signing-key=" << toNarrow(modSigningKey) << "\n";
    if (!modPublicKey.empty())
        file << "mod-public-key=" << toNarrow(modPublicKey) << "\n";
    file << "\n# Resource Packs\n";
    file << "force-server-packs=" << (forceServerPacks ? "true" : "false") << "\n";
    file << "resource-pack-max-size=" << resourcePackMaxSize << "\n";
    file << "\n# Transfer\n";
    file << "transfer-chunk-size=" << transferChunkSize << "\n";
    file << "max-concurrent-transfers=" << maxConcurrentTransfers << "\n";
}
