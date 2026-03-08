#include "TokenStorage.h"

#include <Windows.h>
#include <dpapi.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <vector>
#include <iterator>

#pragma comment(lib, "crypt32.lib")

using json = nlohmann::json;

bool TokenStorage::Save(const std::string& path, const AuthTokens& tokens) {
    json j;
    j["microsoftAccessToken"] = tokens.microsoftAccessToken;
    j["microsoftRefreshToken"] = tokens.microsoftRefreshToken;
    j["xboxLiveToken"] = tokens.xboxLiveToken;
    j["xboxUserHash"] = tokens.xboxUserHash;
    j["xstsToken"] = tokens.xstsToken;
    j["minecraftToken"] = tokens.minecraftToken;
    j["username"] = tokens.username;
    j["uuid"] = tokens.uuid;
    j["expiresAt"] = tokens.expiresAt;

    std::string plaintext = j.dump();
    auto encrypted = Encrypt(plaintext);
    if (encrypted.empty()) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(encrypted.data()),
               static_cast<std::streamsize>(encrypted.size()));
    return file.good();
}

bool TokenStorage::Load(const std::string& path, AuthTokens& tokens) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    std::vector<uint8_t> encrypted(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    if (encrypted.empty()) return false;

    std::string plaintext = Decrypt(encrypted);
    if (plaintext.empty()) return false;

    try {
        auto j = json::parse(plaintext);
        tokens.microsoftAccessToken = j.value("microsoftAccessToken", "");
        tokens.microsoftRefreshToken = j.value("microsoftRefreshToken", "");
        tokens.xboxLiveToken = j.value("xboxLiveToken", "");
        tokens.xboxUserHash = j.value("xboxUserHash", "");
        tokens.xstsToken = j.value("xstsToken", "");
        tokens.minecraftToken = j.value("minecraftToken", "");
        tokens.username = j.value("username", "");
        tokens.uuid = j.value("uuid", "");
        tokens.expiresAt = j.value("expiresAt", static_cast<int64_t>(0));
        return true;
    } catch (...) {
        return false;
    }
}

bool TokenStorage::Delete(const std::string& path) {
    return DeleteFileA(path.c_str()) != 0;
}

std::vector<uint8_t> TokenStorage::Encrypt(const std::string& plaintext) {
    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plaintext.data()));
    dataIn.cbData = static_cast<DWORD>(plaintext.size());

    DATA_BLOB dataOut;
    if (!CryptProtectData(&dataIn, L"OurCraft Tokens", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        return {};
    }

    std::vector<uint8_t> result(dataOut.pbData, dataOut.pbData + dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}

std::string TokenStorage::Decrypt(const std::vector<uint8_t>& ciphertext) {
    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<uint8_t*>(ciphertext.data()));
    dataIn.cbData = static_cast<DWORD>(ciphertext.size());

    DATA_BLOB dataOut;
    if (!CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        return "";
    }

    std::string result(reinterpret_cast<char*>(dataOut.pbData), dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}
