#include "AssetManager.h"
#include "BedrockExtractor.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <SDL3/SDL.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

static const char* ASSET_VERSION = "1.0.0";

bool AssetManager::AssetsUpToDate(const std::string& installDir) const
{
    fs::path manifestPath = fs::path(installDir) / "assets" / "manifest.json";
    if (!fs::exists(manifestPath))
        return false;

    try {
        std::ifstream f(manifestPath);
        json manifest = json::parse(f);
        std::string version = manifest.value("version", "");
        return version == ASSET_VERSION;
    } catch (const std::exception& e) {
        SDL_Log("AssetManager: Failed to read manifest: %s", e.what());
        return false;
    }
}

bool AssetManager::SyncAssets(const Config& config,
                              std::function<void(const std::string&, float)> progress)
{
    fs::path assetsDir = fs::path(config.installDir) / "assets";

    if (AssetsUpToDate(config.installDir)) {
        SDL_Log("AssetManager: Assets are up to date");
        if (progress) progress("Assets up to date", 1.0f);
        return true;
    }

    // Try extracting from local Bedrock installation first
    if (progress) progress("Searching for Bedrock installation...", 0.0f);

    std::string bedrockPath = BedrockExtractor::FindBedrockInstall();
    if (!bedrockPath.empty()) {
        SDL_Log("AssetManager: Found Bedrock at %s, extracting assets...", bedrockPath.c_str());

        if (BedrockExtractor::ExtractAssets(bedrockPath, assetsDir.string(), progress)) {
            // Write manifest
            json manifest;
            manifest["version"] = ASSET_VERSION;
            manifest["source"] = "bedrock_local";
            manifest["bedrockPath"] = bedrockPath;
            manifest["timestamp"] = static_cast<int64_t>(std::time(nullptr));

            try {
                fs::create_directories(assetsDir);
                std::ofstream f(assetsDir / "manifest.json");
                f << manifest.dump(2);
                SDL_Log("AssetManager: Asset sync complete (from Bedrock)");
                return true;
            } catch (const std::exception& e) {
                SDL_Log("AssetManager: Failed to write manifest: %s", e.what());
            }
        }
    }

    // Fall back to CDN download
    SDL_Log("AssetManager: No local Bedrock found, trying CDN...");
    return DownloadFromCDN(config, progress);
}

bool AssetManager::SyncDLC(const Config& config, const std::vector<std::string>& entitlements,
                           std::function<void(const std::string&, float)> progress)
{
    if (entitlements.empty()) {
        SDL_Log("AssetManager: No DLC entitlements to sync");
        return true;
    }

    fs::path dlcDir = fs::path(config.installDir) / "assets" / "dlc";
    try {
        fs::create_directories(dlcDir);
    } catch (const std::exception& e) {
        SDL_Log("AssetManager: Cannot create DLC dir: %s", e.what());
        return false;
    }

    // Try to find packs from local Bedrock
    std::string bedrockPath = BedrockExtractor::FindBedrockInstall();
    std::vector<std::string> localPacks;
    if (!bedrockPath.empty()) {
        localPacks = BedrockExtractor::ListLocalPacks(bedrockPath);
    }

    int completed = 0;
    int total = static_cast<int>(entitlements.size());

    for (const auto& entitlement : entitlements) {
        if (progress) {
            progress("Syncing DLC: " + entitlement,
                     static_cast<float>(completed) / total);
        }

        fs::path entitlementDir = dlcDir / entitlement;
        if (fs::exists(entitlementDir)) {
            SDL_Log("AssetManager: DLC '%s' already present", entitlement.c_str());
            completed++;
            continue;
        }

        // Check if available locally
        bool foundLocal = false;
        for (const auto& pack : localPacks) {
            if (pack == entitlement) {
                // Try to copy from Bedrock
                std::vector<fs::path> searchPaths = {
                    fs::path(bedrockPath) / "resource_packs" / pack,
                    fs::path(bedrockPath) / "data" / "resource_packs" / pack,
                    fs::path(bedrockPath) / "skin_packs" / pack,
                    fs::path(bedrockPath) / "data" / "skin_packs" / pack
                };

                for (const auto& src : searchPaths) {
                    if (fs::exists(src)) {
                        try {
                            fs::create_directories(entitlementDir);
                            fs::copy(src, entitlementDir,
                                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                            SDL_Log("AssetManager: Copied DLC '%s' from Bedrock", entitlement.c_str());
                            foundLocal = true;
                            break;
                        } catch (const std::exception& e) {
                            SDL_Log("AssetManager: Failed to copy DLC '%s': %s",
                                    entitlement.c_str(), e.what());
                        }
                    }
                }
                break;
            }
        }

        if (!foundLocal) {
            // TODO: Download from CDN when available
            SDL_Log("AssetManager: DLC '%s' not available locally and CDN not implemented",
                    entitlement.c_str());
        }

        completed++;
    }

    if (progress) progress("DLC sync complete", 1.0f);
    return true;
}

bool AssetManager::DownloadFromCDN(const Config& config,
                                   std::function<void(const std::string&, float)> progress)
{
    // TODO: Implement CDN download when server infrastructure is ready
    // This would download a zip of assets from our CDN, verify checksum, and extract
    SDL_Log("AssetManager: CDN download not yet implemented");
    if (progress) progress("CDN download not available", 0.0f);
    return false;
}
