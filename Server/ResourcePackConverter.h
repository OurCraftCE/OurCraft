#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace ResourcePackConverter {

// convert Bedrock .mcpack/.zip to Java resource pack format; empty on failure
std::vector<uint8_t> convertBedrockToJava(const std::string& bedrockPackPath,
                                           const std::string& cacheDir);

bool hasCachedConversion(const std::string& bedrockPackPath,
                          const std::string& cacheDir);

std::string getCachePath(const std::string& bedrockPackPath,
                          const std::string& cacheDir);

}
