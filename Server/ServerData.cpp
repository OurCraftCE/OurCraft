#include "ServerData.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ServerData& ServerData::instance() {
    static ServerData s;
    return s;
}

ServerData::ServerData() {
    initDefaultKits();
}

void ServerData::initDefaultKits() {
    m_kits.push_back({"starter", {  // wood tools + food
        {268, 1, 0},  // wood sword
        {270, 1, 0},  // wood pickaxe
        {271, 1, 0},  // wood axe
        {269, 1, 0},  // wood shovel
        {297, 16, 0}, // bread
        {50, 32, 0},  // torches
    }});

    m_kits.push_back({"tools", {  // iron tools
        {267, 1, 0},  // iron sword
        {257, 1, 0},  // iron pickaxe
        {258, 1, 0},  // iron axe
        {256, 1, 0},  // iron shovel
        {359, 1, 0},  // shears
        {325, 1, 0},  // bucket
    }});

    m_kits.push_back({"combat", {  // diamond armor + sword
        {276, 1, 0},  // diamond sword
        {261, 1, 0},  // bow
        {262, 64, 0}, // arrows
        {310, 1, 0},  // diamond helmet
        {311, 1, 0},  // diamond chestplate
        {312, 1, 0},  // diamond leggings
        {313, 1, 0},  // diamond boots
        {322, 3, 0},  // golden apples
    }});

    m_kits.push_back({"building", {  // common blocks
        {4, 64, 0},   // cobblestone
        {5, 64, 0},   // planks
        {20, 64, 0},  // glass
        {85, 64, 0},  // fence
        {50, 64, 0},  // torches
        {65, 16, 0},  // ladders
        {324, 1, 0},  // wooden door
    }});

    m_kits.push_back({"farming", {
        {291, 1, 0},  // iron hoe
        {295, 16, 0}, // wheat seeds
        {361, 8, 0},  // pumpkin seeds
        {362, 8, 0},  // melon seeds
        {325, 1, 0},  // bucket
        {351, 16, 15},// bone meal
    }});

    m_kits.push_back({"redstone", {
        {331, 64, 0}, // redstone dust
        {76, 16, 0},  // redstone torch
        {356, 8, 0},  // repeater
        {69, 8, 0},   // lever
        {77, 8, 0},   // button
        {33, 8, 0},   // piston
        {29, 8, 0},   // sticky piston
    }});
}

bool ServerData::setHome(const std::string& player, const Position& pos) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_homes[player] = pos;
    return true;
}

bool ServerData::getHome(const std::string& player, Position& out) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_homes.find(player);
    if (it == m_homes.end()) return false;
    out = it->second;
    return true;
}

bool ServerData::delHome(const std::string& player) {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_homes.erase(player) > 0;
}

bool ServerData::setWarp(const std::string& name, const Position& pos) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_warps[name] = pos;
    return true;
}

bool ServerData::getWarp(const std::string& name, Position& out) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_warps.find(name);
    if (it == m_warps.end()) return false;
    out = it->second;
    return true;
}

bool ServerData::delWarp(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_warps.erase(name) > 0;
}

std::vector<std::string> ServerData::listWarps() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    std::vector<std::string> names;
    for (const auto& kv : m_warps) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    return names;
}

void ServerData::setBack(const std::string& player, const Position& pos) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_back[player] = pos;
}

bool ServerData::getBack(const std::string& player, Position& out) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_back.find(player);
    if (it == m_back.end()) return false;
    out = it->second;
    return true;
}

const Kit* ServerData::getKit(const std::string& name) const {
    for (const auto& k : m_kits) {
        if (k.name == name) return &k;
    }
    return nullptr;
}

std::vector<std::string> ServerData::listKits() const {
    std::vector<std::string> names;
    for (const auto& k : m_kits) names.push_back(k.name);
    return names;
}

// format: "type key x y z yaw pitch dim" per line
void ServerData::load(const char* dir) {
    std::string path = std::string(dir) + "/server_data.txt";
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::lock_guard<std::mutex> lock(m_mtx);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string type, key;
        Position pos = {};
        ss >> type >> key >> pos.x >> pos.y >> pos.z >> pos.yaw >> pos.pitch >> pos.dimension;
        if (ss.fail()) continue;

        if (type == "home") m_homes[key] = pos;
        else if (type == "warp") m_warps[key] = pos;
    }
}

void ServerData::save(const char* dir) const {
    std::string path = std::string(dir) + "/server_data.txt";
    std::ofstream file(path);
    if (!file.is_open()) return;

    std::lock_guard<std::mutex> lock(m_mtx);
    file << "# Ourcraft Server Data (homes and warps)\n";
    for (const auto& kv : m_homes) {
        file << "home " << kv.first << " "
             << kv.second.x << " " << kv.second.y << " " << kv.second.z << " "
             << kv.second.yaw << " " << kv.second.pitch << " " << kv.second.dimension << "\n";
    }
    for (const auto& kv : m_warps) {
        file << "warp " << kv.first << " "
             << kv.second.x << " " << kv.second.y << " " << kv.second.z << " "
             << kv.second.yaw << " " << kv.second.pitch << " " << kv.second.dimension << "\n";
    }
}
