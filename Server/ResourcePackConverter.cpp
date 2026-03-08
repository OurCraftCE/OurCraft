#include "ResourcePackConverter.h"
#include "CryptoUtils.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <miniz.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace ResourcePackConverter {

static std::string computeCacheKey(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return "";
    size_t size = (size_t)f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()), size);
    auto hash = CryptoUtils::hash256(data.data(), data.size());
    char hex[65];
    for (int i = 0; i < 32; i++) snprintf(hex + i * 2, 3, "%02x", hash[i]);
    hex[64] = 0;
    return hex;
}

std::string getCachePath(const std::string& bedrockPackPath, const std::string& cacheDir) {
    std::string key = computeCacheKey(bedrockPackPath);
    if (key.empty()) return "";
    return cacheDir + "/" + key + "_java.zip";
}

bool hasCachedConversion(const std::string& bedrockPackPath, const std::string& cacheDir) {
    std::string path = getCachePath(bedrockPackPath, cacheDir);
    return !path.empty() && fs::exists(path);
}

// remap Bedrock asset paths to Java pack paths
static std::string remapPath(const std::string& bedrockPath) {
    if (bedrockPath.find("textures/blocks/") == 0) {
        return "assets/minecraft/textures/block/" + bedrockPath.substr(16);
    }
    if (bedrockPath.find("textures/items/") == 0) {
        return "assets/minecraft/textures/item/" + bedrockPath.substr(15);
    }
    if (bedrockPath.find("textures/entity/") == 0) {
        return "assets/minecraft/textures/entity/" + bedrockPath.substr(16);
    }
    if (bedrockPath.find("textures/environment/") == 0) {
        return "assets/minecraft/textures/environment/" + bedrockPath.substr(21);
    }
    if (bedrockPath.find("textures/painting/") == 0) {
        return "assets/minecraft/textures/painting/" + bedrockPath.substr(18);
    }
    if (bedrockPath.find("textures/particle/") == 0) {
        return "assets/minecraft/textures/particle/" + bedrockPath.substr(18);
    }
    if (bedrockPath.find("textures/gui/") == 0) {
        return "assets/minecraft/textures/gui/" + bedrockPath.substr(13);
    }
    if (bedrockPath.find("sounds/") == 0) {
        return "assets/minecraft/sounds/" + bedrockPath.substr(7);
    }
    if (bedrockPath.find("textures/") == 0) {
        return "assets/minecraft/" + bedrockPath;
    }
    return ""; // skip non-asset files (manifest.json, etc.)
}

static std::string generatePackMcmeta(const std::string& manifestJson) {
    try {
        auto manifest = json::parse(manifestJson);
        std::string desc = "Converted Bedrock Pack";
        if (manifest.contains("header") && manifest["header"].contains("description")) {
            desc = manifest["header"]["description"].get<std::string>();
        }
        json mcmeta;
        mcmeta["pack"]["pack_format"] = 1; // 1.8.x format
        mcmeta["pack"]["description"] = desc;
        return mcmeta.dump(2);
    } catch (...) {
        json mcmeta;
        mcmeta["pack"]["pack_format"] = 1;
        mcmeta["pack"]["description"] = "Converted Resource Pack";
        return mcmeta.dump(2);
    }
}

std::vector<uint8_t> convertBedrockToJava(const std::string& bedrockPackPath,
                                           const std::string& cacheDir) {
    fs::create_directories(cacheDir);
    std::string cachePath = getCachePath(bedrockPackPath, cacheDir);
    if (!cachePath.empty() && fs::exists(cachePath)) {
        std::ifstream cf(cachePath, std::ios::binary | std::ios::ate);
        if (cf.is_open()) {
            size_t sz = (size_t)cf.tellg();
            cf.seekg(0);
            std::vector<uint8_t> cached(sz);
            cf.read(reinterpret_cast<char*>(cached.data()), sz);
            printf("[PackConvert] Using cached: %s\n", cachePath.c_str());
            return cached;
        }
    }

    printf("[PackConvert] Converting: %s\n", bedrockPackPath.c_str());

    mz_zip_archive srcZip;
    memset(&srcZip, 0, sizeof(srcZip));
    if (!mz_zip_reader_init_file(&srcZip, bedrockPackPath.c_str(), 0)) {
        printf("[PackConvert] Failed to open: %s\n", bedrockPackPath.c_str());
        return {};
    }

    mz_zip_archive dstZip;
    memset(&dstZip, 0, sizeof(dstZip));
    if (!mz_zip_writer_init_heap(&dstZip, 0, 0)) {
        mz_zip_reader_end(&srcZip);
        printf("[PackConvert] Failed to init output zip\n");
        return {};
    }

    int numFiles = (int)mz_zip_reader_get_num_files(&srcZip);
    bool hasManifest = false;
    int convertedFiles = 0;

    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&srcZip, i, &stat)) continue;
        if (stat.m_is_directory) continue;

        std::string filename = stat.m_filename;

        size_t fileSize = 0;
        void* fileData = mz_zip_reader_extract_to_heap(&srcZip, i, &fileSize, 0);
        if (!fileData) continue;

        if (filename == "manifest.json") {
            hasManifest = true;
            std::string manifestStr((const char*)fileData, fileSize);
            std::string mcmeta = generatePackMcmeta(manifestStr);
            mz_zip_writer_add_mem(&dstZip, "pack.mcmeta",
                                   mcmeta.data(), mcmeta.size(), MZ_DEFAULT_COMPRESSION);
            convertedFiles++;
        }
        else if (filename == "pack_icon.png") {
            mz_zip_writer_add_mem(&dstZip, "pack.png",
                                   fileData, fileSize, MZ_DEFAULT_COMPRESSION);
            convertedFiles++;
        }
        else {
            std::string javaPath = remapPath(filename);
            if (!javaPath.empty()) {
                mz_zip_writer_add_mem(&dstZip, javaPath.c_str(),
                                       fileData, fileSize, MZ_DEFAULT_COMPRESSION);
                convertedFiles++;
            }
        }

        mz_free(fileData);
    }

    mz_zip_reader_end(&srcZip);

    if (!hasManifest) {
        std::string defaultMcmeta = generatePackMcmeta("{}");
        mz_zip_writer_add_mem(&dstZip, "pack.mcmeta",
                               defaultMcmeta.data(), defaultMcmeta.size(), MZ_DEFAULT_COMPRESSION);
    }

    void* outBuf = nullptr;
    size_t outSize = 0;
    if (!mz_zip_writer_finalize_heap_archive(&dstZip, &outBuf, &outSize)) {
        mz_zip_writer_end(&dstZip);
        printf("[PackConvert] Failed to finalize output zip\n");
        return {};
    }

    std::vector<uint8_t> result((uint8_t*)outBuf, (uint8_t*)outBuf + outSize);
    mz_free(outBuf);
    mz_zip_writer_end(&dstZip);

    printf("[PackConvert] Converted %d files -> %zu bytes\n", convertedFiles, result.size());

    if (!cachePath.empty()) {
        std::ofstream cf(cachePath, std::ios::binary);
        if (cf.is_open()) {
            cf.write(reinterpret_cast<const char*>(result.data()), result.size());
            printf("[PackConvert] Cached to: %s\n", cachePath.c_str());
        }
    }

    return result;
}

} // namespace ResourcePackConverter
