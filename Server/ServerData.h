#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

struct Position {
    double x, y, z;
    float yaw, pitch;
    int dimension;
};

struct KitItem {
    int id;
    int count;
    int damage;
};

struct Kit {
    std::string name;
    std::vector<KitItem> items;
};

// persistent server data: homes, warps, kits, back positions
class ServerData {
public:
    static ServerData& instance();

    bool setHome(const std::string& player, const Position& pos);
    bool getHome(const std::string& player, Position& out) const;
    bool delHome(const std::string& player);

    bool setWarp(const std::string& name, const Position& pos);
    bool getWarp(const std::string& name, Position& out) const;
    bool delWarp(const std::string& name);
    std::vector<std::string> listWarps() const;

    void setBack(const std::string& player, const Position& pos);
    bool getBack(const std::string& player, Position& out) const;

    const Kit* getKit(const std::string& name) const;
    std::vector<std::string> listKits() const;

    void load(const char* dir);
    void save(const char* dir) const;

private:
    ServerData();
    void initDefaultKits();

    mutable std::mutex m_mtx;
    std::unordered_map<std::string, Position> m_homes;
    std::unordered_map<std::string, Position> m_warps;
    std::unordered_map<std::string, Position> m_back;
    std::vector<Kit> m_kits;
};
