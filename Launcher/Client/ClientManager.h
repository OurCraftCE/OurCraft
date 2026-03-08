#pragma once
#include <string>
#include <functional>

class ClientManager {
public:
    static constexpr const char* DEFAULT_RELEASES_URL =
        "https://codeberg.org/api/v1/repos/OWNER/REPO/releases/latest";

    struct Config {
        std::string installDir;
        bool devMode = false;
        std::string devBuildDir;
    };

    void SetReleasesUrl(const std::string& url) { m_ReleasesUrl = url; }

    bool CheckForUpdate(const Config& config, std::string& latestVersion);
    bool UpdateClient(const Config& config, const std::string& version,
                      std::function<void(const std::string&, float)> progress = nullptr);
    bool Launch(const Config& config, const std::string& assetsDir,
                const std::string& userToken, const std::string& username);
    std::string GetInstalledVersion(const std::string& installDir) const;
    std::string FindDevBuild() const;

private:
    std::string m_ReleasesUrl = DEFAULT_RELEASES_URL;
};
