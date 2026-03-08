#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct FlipbookEntry
{
    std::wstring atlasTexture;
    std::wstring flipbookTexture;
    int ticksPerFrame = 1;
    std::vector<int> frames;
    int replicate = 0;
};

class BedrockResourcePack
{
public:
    bool load(const std::wstring &rootPath);
    std::wstring getTerrainTexturePath(const std::wstring &shortName) const;
    std::wstring getItemTexturePath(const std::wstring &shortName) const;
    const std::unordered_map<std::wstring, std::wstring> &getTerrainTextures() const;
    const std::unordered_map<std::wstring, std::wstring> &getItemTextures() const;
    const std::vector<FlipbookEntry> &getFlipbooks() const;
    std::wstring getName() const;
    std::wstring getDescription() const;
    int getFormatVersion() const;
    bool isLoaded() const { return m_loaded; }

private:
    bool parseManifest(const std::wstring &rootPath);
    bool parseTerrainTexture(const std::wstring &rootPath);
    bool parseItemTexture(const std::wstring &rootPath);
    bool parseFlipbookTextures(const std::wstring &rootPath);
    std::string readFileToString(const std::wstring &path);

    std::unordered_map<std::wstring, std::wstring> m_terrainTextures;
    std::unordered_map<std::wstring, std::wstring> m_itemTextures;
    std::vector<FlipbookEntry> m_flipbooks;
    std::wstring m_name;
    std::wstring m_description;
    int m_formatVersion = 0;
    bool m_loaded = false;
};
