#include "BedrockExtractor.h"
#include <filesystem>
#include <SDL3/SDL.h>

namespace fs = std::filesystem;

std::string BedrockExtractor::FindBedrockInstall()
{
    // Check XboxGames path first (most accessible)
    std::vector<std::string> xboxPaths = {
        "C:/XboxGames/Minecraft/Content",
        "D:/XboxGames/Minecraft/Content",
        "E:/XboxGames/Minecraft/Content"
    };

    for (const auto& path : xboxPaths) {
        try {
            if (fs::exists(path)) {
                SDL_Log("BedrockExtractor: Found Bedrock at %s", path.c_str());
                return path;
            }
        } catch (const std::exception&) {}
    }

    // Search WindowsApps for Microsoft.MinecraftUWP_*
    std::vector<std::string> windowsAppsDirs = {
        "C:/Program Files/WindowsApps",
        "D:/Program Files/WindowsApps",
        "C:/Windows/WindowsApps"
    };

    for (const auto& appsDir : windowsAppsDirs) {
        try {
            if (!fs::exists(appsDir))
                continue;

            for (const auto& entry : fs::directory_iterator(appsDir)) {
                if (!entry.is_directory())
                    continue;

                std::string dirName = entry.path().filename().string();
                if (dirName.find("Microsoft.MinecraftUWP_") == 0) {
                    std::string found = entry.path().string();
                    SDL_Log("BedrockExtractor: Found Bedrock UWP at %s", found.c_str());
                    return found;
                }
            }
        } catch (const std::exception& e) {
            SDL_Log("BedrockExtractor: Cannot access %s: %s", appsDir.c_str(), e.what());
        }
    }

    SDL_Log("BedrockExtractor: No Bedrock installation found");
    return {};
}

bool BedrockExtractor::ExtractAssets(const std::string& bedrockPath, const std::string& targetDir,
                                     std::function<void(const std::string&, float)> progress)
{
    if (bedrockPath.empty()) {
        SDL_Log("BedrockExtractor: No Bedrock path provided");
        return false;
    }

    // Locate vanilla resource pack
    fs::path vanillaBase = fs::path(bedrockPath) / "data" / "resource_packs" / "vanilla";
    if (!fs::exists(vanillaBase)) {
        // Try alternate location for XboxGames installs
        vanillaBase = fs::path(bedrockPath) / "resource_packs" / "vanilla";
        if (!fs::exists(vanillaBase)) {
            SDL_Log("BedrockExtractor: Vanilla resource pack not found in %s", bedrockPath.c_str());
            return false;
        }
    }

    // Directories to extract
    struct CopyEntry {
        std::string subdir;
        std::string description;
    };
    std::vector<CopyEntry> entries = {
        { "textures",  "Textures" },
        { "sounds",    "Sounds" },
        { "texts",     "Language files" },
        { "font",      "Fonts" },
        { "models",    "Models" },
        { "particles", "Particles" }
    };

    fs::path target = fs::path(targetDir);

    try {
        fs::create_directories(target);
    } catch (const std::exception& e) {
        SDL_Log("BedrockExtractor: Cannot create target dir: %s", e.what());
        return false;
    }

    int completed = 0;
    int total = static_cast<int>(entries.size());

    for (const auto& entry : entries) {
        fs::path src = vanillaBase / entry.subdir;
        fs::path dst = target / entry.subdir;

        if (progress) {
            progress(entry.description, static_cast<float>(completed) / total);
        }

        if (!fs::exists(src)) {
            SDL_Log("BedrockExtractor: Skipping missing dir: %s", src.string().c_str());
            completed++;
            continue;
        }

        try {
            fs::create_directories(dst);
            fs::copy(src, dst,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            SDL_Log("BedrockExtractor: Copied %s", entry.subdir.c_str());
        } catch (const std::exception& e) {
            SDL_Log("BedrockExtractor: Failed to copy %s: %s", entry.subdir.c_str(), e.what());
        }

        completed++;
    }

    // Also copy manifest/pack_manifest if present
    for (const char* manifest : { "manifest.json", "pack_manifest.json" }) {
        fs::path mf = vanillaBase / manifest;
        if (fs::exists(mf)) {
            try {
                fs::copy_file(mf, target / manifest, fs::copy_options::overwrite_existing);
            } catch (...) {}
        }
    }

    if (progress) {
        progress("Complete", 1.0f);
    }

    SDL_Log("BedrockExtractor: Asset extraction complete");
    return true;
}

std::vector<std::string> BedrockExtractor::ListLocalPacks(const std::string& bedrockPath)
{
    std::vector<std::string> packs;
    if (bedrockPath.empty())
        return packs;

    std::vector<std::string> packDirs = { "resource_packs", "skin_packs" };
    // Also check under data/
    std::vector<fs::path> bases = {
        fs::path(bedrockPath),
        fs::path(bedrockPath) / "data"
    };

    for (const auto& base : bases) {
        for (const auto& dir : packDirs) {
            fs::path packRoot = base / dir;
            try {
                if (!fs::exists(packRoot))
                    continue;

                for (const auto& entry : fs::directory_iterator(packRoot)) {
                    if (!entry.is_directory())
                        continue;

                    std::string name = entry.path().filename().string();
                    // Skip vanilla pack
                    if (name == "vanilla" || name == "vanilla_base")
                        continue;

                    packs.push_back(name);
                }
            } catch (const std::exception&) {}
        }
    }

    return packs;
}
