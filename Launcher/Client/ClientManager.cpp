#include "ClientManager.h"
#include "../Auth/HttpClient.h"
#include <nlohmann/json.hpp>
#include <miniz/miniz.h>
#include <filesystem>
#include <fstream>
#include <SDL3/SDL.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Codeberg/Gitea API: GET /api/v1/repos/{owner}/{repo}/releases/latest
// GitHub API:         GET /repos/{owner}/{repo}/releases/latest
// Both return similar JSON with "tag_name" and "assets" array

std::string ClientManager::GetInstalledVersion(const std::string& installDir) const
{
    fs::path versionFile = fs::path(installDir) / "version.json";
    if (!fs::exists(versionFile))
        return {};

    try {
        std::ifstream f(versionFile);
        json ver = json::parse(f);
        return ver.value("version", "");
    } catch (const std::exception& e) {
        SDL_Log("ClientManager: Failed to read version.json: %s", e.what());
        return {};
    }
}

bool ClientManager::CheckForUpdate(const Config& config, std::string& latestVersion)
{
    HttpClient http;
    auto resp = http.Get(m_ReleasesUrl, {
        { "Accept", "application/vnd.github.v3+json" },
        { "User-Agent", "OurCraft-Launcher/1.0" }
    });

    if (resp.statusCode != 200) {
        SDL_Log("ClientManager: Failed to check for updates (HTTP %ld)", resp.statusCode);
        return false;
    }

    try {
        json release = json::parse(resp.body);
        latestVersion = release.value("tag_name", "");

        std::string installed = GetInstalledVersion(config.installDir);
        if (latestVersion.empty()) {
            SDL_Log("ClientManager: No tag_name in release response");
            return false;
        }

        bool updateAvailable = (latestVersion != installed);
        SDL_Log("ClientManager: Installed=%s Latest=%s Update=%s",
                installed.c_str(), latestVersion.c_str(),
                updateAvailable ? "yes" : "no");
        return updateAvailable;
    } catch (const std::exception& e) {
        SDL_Log("ClientManager: Failed to parse release info: %s", e.what());
        return false;
    }
}

bool ClientManager::UpdateClient(const Config& config, const std::string& version,
                                 std::function<void(const std::string&, float)> progress)
{
    if (progress) progress("Checking release...", 0.0f);

    // Fetch release info to get asset download URL
    HttpClient http;
    auto resp = http.Get(m_ReleasesUrl, {
        { "Accept", "application/vnd.github.v3+json" },
        { "User-Agent", "OurCraft-Launcher/1.0" }
    });

    if (resp.statusCode != 200) {
        SDL_Log("ClientManager: Failed to fetch release (HTTP %ld)", resp.statusCode);
        return false;
    }

    std::string downloadUrl;
    try {
        json release = json::parse(resp.body);
        auto& assets = release["assets"];
        for (const auto& asset : assets) {
            std::string name = asset.value("name", "");
            if (name.find(".zip") != std::string::npos) {
                // GitHub uses "browser_download_url", Codeberg/Gitea uses "browser_download_url" too
                downloadUrl = asset.value("browser_download_url", "");
                break;
            }
        }
    } catch (const std::exception& e) {
        SDL_Log("ClientManager: Failed to parse release: %s", e.what());
        return false;
    }

    if (downloadUrl.empty()) {
        SDL_Log("ClientManager: No zip asset found in release");
        return false;
    }

    // Download zip
    fs::path versionDir = fs::path(config.installDir) / "versions" / version;
    fs::create_directories(versionDir);

    fs::path zipPath = versionDir / "client.zip";

    if (progress) progress("Downloading...", 0.1f);

    bool downloaded = http.Download(downloadUrl, zipPath.string(),
        [&progress](size_t downloaded, size_t total) {
            if (progress && total > 0) {
                float pct = 0.1f + 0.7f * (static_cast<float>(downloaded) / total);
                progress("Downloading...", pct);
            }
        });

    if (!downloaded) {
        SDL_Log("ClientManager: Download failed");
        return false;
    }

    // Extract zip with miniz
    if (progress) progress("Extracting...", 0.8f);

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_file(&zip, zipPath.string().c_str(), 0)) {
        SDL_Log("ClientManager: Failed to open zip");
        return false;
    }

    int numFiles = static_cast<int>(mz_zip_reader_get_num_files(&zip));
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zip, i, &fileStat))
            continue;

        fs::path outPath = versionDir / fileStat.m_filename;

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            fs::create_directories(outPath);
        } else {
            fs::create_directories(outPath.parent_path());
            if (!mz_zip_reader_extract_to_file(&zip, i, outPath.string().c_str(), 0)) {
                SDL_Log("ClientManager: Failed to extract %s", fileStat.m_filename);
            }
        }
    }

    mz_zip_reader_end(&zip);

    // Clean up zip
    try { fs::remove(zipPath); } catch (...) {}

    // Write version.json
    json versionJson;
    versionJson["version"] = version;
    versionJson["timestamp"] = static_cast<int64_t>(std::time(nullptr));
    versionJson["path"] = versionDir.string();

    try {
        std::ofstream f(fs::path(config.installDir) / "version.json");
        f << versionJson.dump(2);
    } catch (const std::exception& e) {
        SDL_Log("ClientManager: Failed to write version.json: %s", e.what());
    }

    if (progress) progress("Update complete", 1.0f);
    SDL_Log("ClientManager: Updated to version %s", version.c_str());
    return true;
}

std::string ClientManager::FindDevBuild() const
{
    // Search common build output locations
    std::vector<std::string> searchPaths = {
        "x64/Debug/Minecraft.Client.exe",
        "x64/Release/Minecraft.Client.exe",
        "../x64/Debug/Minecraft.Client.exe",
        "../x64/Release/Minecraft.Client.exe",
        "Debug/Minecraft.Client.exe",
        "Release/Minecraft.Client.exe",
    };

    for (const auto& path : searchPaths) {
        try {
            fs::path p = fs::absolute(path);
            if (fs::exists(p)) {
                SDL_Log("ClientManager: Found dev build at %s", p.string().c_str());
                return p.string();
            }
        } catch (...) {}
    }

    return {};
}

bool ClientManager::Launch(const Config& config, const std::string& assetsDir,
                           const std::string& userToken, const std::string& username)
{
    std::string exePath;

    if (config.devMode) {
        // In dev mode, use devBuildDir or search for it
        if (!config.devBuildDir.empty()) {
            exePath = (fs::path(config.devBuildDir) / "Minecraft.Client.exe").string();
        } else {
            exePath = FindDevBuild();
        }
    } else {
        // Use installed version
        std::string version = GetInstalledVersion(config.installDir);
        if (version.empty()) {
            SDL_Log("ClientManager: No installed version found");
            return false;
        }
        exePath = (fs::path(config.installDir) / "versions" / version / "Minecraft.Client.exe").string();
    }

    if (exePath.empty() || !fs::exists(exePath)) {
        SDL_Log("ClientManager: Client executable not found: %s", exePath.c_str());
        return false;
    }

    // Build command line
    std::string cmdLine = "\"" + exePath + "\""
        + " --assets-dir \"" + assetsDir + "\""
        + " --user-token \"" + userToken + "\""
        + " --username \"" + username + "\"";

    // Working directory = exe parent dir
    std::string workDir = fs::path(exePath).parent_path().string();

    SDL_Log("ClientManager: Launching %s", exePath.c_str());
    SDL_Log("ClientManager: Working dir: %s", workDir.c_str());

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    // CreateProcessA needs a mutable command line buffer
    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    BOOL result = CreateProcessA(
        nullptr,
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workDir.c_str(),
        &si,
        &pi
    );

    if (!result) {
        DWORD err = GetLastError();
        SDL_Log("ClientManager: CreateProcess failed with error %lu", err);
        return false;
    }

    // Don't wait for the process - close handles and return
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    SDL_Log("ClientManager: Client launched successfully (PID %lu)", pi.dwProcessId);
    return true;
}
