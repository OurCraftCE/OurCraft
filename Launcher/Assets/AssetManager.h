#pragma once
#include <string>
#include <functional>
#include <vector>

class AssetManager {
public:
    struct Config {
        std::string installDir;
        std::string minecraftToken;
    };

    bool SyncAssets(const Config& config, std::function<void(const std::string&, float)> progress = nullptr);
    bool SyncDLC(const Config& config, const std::vector<std::string>& entitlements,
                 std::function<void(const std::string&, float)> progress = nullptr);
    bool AssetsUpToDate(const std::string& installDir) const;

private:
    bool DownloadFromCDN(const Config& config, std::function<void(const std::string&, float)> progress);
};
