#pragma once
#include "MicrosoftAuth.h"
#include <string>
#include <vector>
#include <cstdint>

class TokenStorage {
public:
    static bool Save(const std::string& path, const AuthTokens& tokens);
    static bool Load(const std::string& path, AuthTokens& tokens);
    static bool Delete(const std::string& path);

private:
    static std::vector<uint8_t> Encrypt(const std::string& plaintext);
    static std::string Decrypt(const std::vector<uint8_t>& ciphertext);
};
