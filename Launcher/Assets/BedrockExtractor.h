#pragma once
#include <string>
#include <vector>
#include <functional>

class BedrockExtractor {
public:
    static std::string FindBedrockInstall();
    static bool ExtractAssets(const std::string& bedrockPath, const std::string& targetDir,
                              std::function<void(const std::string&, float)> progress = nullptr);
    static std::vector<std::string> ListLocalPacks(const std::string& bedrockPath);
};
