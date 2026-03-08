#include "stdafx.h"
#include "BedrockResourcePack.h"
#include "Common/nlohmann/json.hpp"
#include "../World/IO/File.h"
#include <fstream>
#include <codecvt>

using json = nlohmann::json;

static std::wstring toWide(const std::string &s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(s);
}

static std::string toNarrow(const std::wstring &s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(s);
}

std::string BedrockResourcePack::readFileToString(const std::wstring &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

bool BedrockResourcePack::load(const std::wstring &rootPath)
{
    m_loaded = false;
    m_terrainTextures.clear();
    m_itemTextures.clear();
    m_flipbooks.clear();

    parseManifest(rootPath);
    parseTerrainTexture(rootPath);
    parseItemTexture(rootPath);
    parseFlipbookTextures(rootPath);

    m_loaded = !m_terrainTextures.empty();
    return m_loaded;
}

bool BedrockResourcePack::parseManifest(const std::wstring &rootPath)
{
    std::string data = readFileToString(rootPath + L"\\manifest.json");
    if (data.empty()) return false;

    try {
        json j = json::parse(data);
        if (j.contains("header")) {
            auto &header = j["header"];
            if (header.contains("name"))
                m_name = toWide(header["name"].get<std::string>());
            if (header.contains("description"))
                m_description = toWide(header["description"].get<std::string>());
            if (header.contains("version") && header["version"].is_array())
                m_formatVersion = header["version"][0].get<int>();
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool BedrockResourcePack::parseTerrainTexture(const std::wstring &rootPath)
{
    std::string data = readFileToString(rootPath + L"\\textures\\terrain_texture.json");
    if (data.empty()) return false;

    try {
        json j = json::parse(data);
        if (!j.contains("texture_data")) return false;

        for (auto item : j["texture_data"].items()) {
            const auto &key = item.key();
            auto &val = item.value();
            std::wstring shortName = toWide(key);
            if (val.contains("textures")) {
                auto &tex = val["textures"];
                std::string texPath;
                if (tex.is_string()) {
                    texPath = tex.get<std::string>();
                } else if (tex.is_object() && tex.contains("path")) {
                    texPath = tex["path"].get<std::string>();
                } else if (tex.is_array() && !tex.empty()) {
                    auto &first = tex[0];
                    if (first.is_string())
                        texPath = first.get<std::string>();
                    else if (first.is_object() && first.contains("path"))
                        texPath = first["path"].get<std::string>();
                }
                if (!texPath.empty())
                    m_terrainTextures[shortName] = toWide(texPath);
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool BedrockResourcePack::parseItemTexture(const std::wstring &rootPath)
{
    std::string data = readFileToString(rootPath + L"\\textures\\item_texture.json");
    if (data.empty()) return false;

    try {
        json j = json::parse(data);
        if (!j.contains("texture_data")) return false;

        for (auto item : j["texture_data"].items()) {
            const auto &key = item.key();
            auto &val = item.value();
            std::wstring shortName = toWide(key);
            if (val.contains("textures")) {
                auto &tex = val["textures"];
                std::string texPath;
                if (tex.is_string()) {
                    texPath = tex.get<std::string>();
                } else if (tex.is_object() && tex.contains("path")) {
                    texPath = tex["path"].get<std::string>();
                } else if (tex.is_array() && !tex.empty()) {
                    auto &first = tex[0];
                    if (first.is_string())
                        texPath = first.get<std::string>();
                    else if (first.is_object() && first.contains("path"))
                        texPath = first["path"].get<std::string>();
                }
                if (!texPath.empty())
                    m_itemTextures[shortName] = toWide(texPath);
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool BedrockResourcePack::parseFlipbookTextures(const std::wstring &rootPath)
{
    std::string data = readFileToString(rootPath + L"\\textures\\flipbook_textures.json");
    if (data.empty()) return false;

    try {
        json j = json::parse(data);
        if (!j.is_array()) return false;

        for (auto &entry : j) {
            FlipbookEntry fb;
            if (entry.contains("flipbook_texture"))
                fb.flipbookTexture = toWide(entry["flipbook_texture"].get<std::string>());
            if (entry.contains("atlas_tile"))
                fb.atlasTexture = toWide(entry["atlas_tile"].get<std::string>());
            fb.ticksPerFrame = entry.value("ticks_per_frame", 1);
            fb.replicate = entry.value("replicate", 0);

            if (entry.contains("frames") && entry["frames"].is_array()) {
                for (auto &f : entry["frames"])
                    fb.frames.push_back(f.get<int>());
            }

            m_flipbooks.push_back(fb);
        }
    } catch (...) {
        return false;
    }
    return true;
}

std::wstring BedrockResourcePack::getTerrainTexturePath(const std::wstring &shortName) const
{
    auto it = m_terrainTextures.find(shortName);
    if (it != m_terrainTextures.end()) return it->second;
    return L"";
}

std::wstring BedrockResourcePack::getItemTexturePath(const std::wstring &shortName) const
{
    auto it = m_itemTextures.find(shortName);
    if (it != m_itemTextures.end()) return it->second;
    return L"";
}

const std::unordered_map<std::wstring, std::wstring> &BedrockResourcePack::getTerrainTextures() const
{
    return m_terrainTextures;
}

const std::unordered_map<std::wstring, std::wstring> &BedrockResourcePack::getItemTextures() const
{
    return m_itemTextures;
}

const std::vector<FlipbookEntry> &BedrockResourcePack::getFlipbooks() const
{
    return m_flipbooks;
}

std::wstring BedrockResourcePack::getName() const { return m_name; }
std::wstring BedrockResourcePack::getDescription() const { return m_description; }
int BedrockResourcePack::getFormatVersion() const { return m_formatVersion; }
