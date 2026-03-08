#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace CryptoUtils {

struct Ed25519KeyPair {
    uint8_t publicKey[32];
    uint8_t privateKey[64];
};

struct X25519KeyPair {
    uint8_t publicKey[32];
    uint8_t secretKey[32];
};

Ed25519KeyPair generateSigningKeyPair();
std::vector<uint8_t> sign(const uint8_t* message, size_t len, const uint8_t privateKey[64]);
bool verify(const uint8_t* message, size_t len, const uint8_t signature[64], const uint8_t publicKey[32]);

X25519KeyPair generateX25519KeyPair();
std::vector<uint8_t> x25519SharedSecret(const uint8_t ourSecret[32], const uint8_t theirPublic[32]);

std::vector<uint8_t> encrypt(const uint8_t* plaintext, size_t len, const uint8_t key[32], const uint8_t nonce[24]);
std::vector<uint8_t> decrypt(const uint8_t* ciphertext, size_t len, const uint8_t key[32], const uint8_t nonce[24]);

std::vector<uint8_t> hash256(const uint8_t* data, size_t len);
void randomBytes(uint8_t* buf, size_t len);

bool saveKeyFile(const char* path, const uint8_t* data, size_t len);
std::vector<uint8_t> loadKeyFile(const char* path);
std::string base64Encode(const uint8_t* data, size_t len);
std::vector<uint8_t> base64Decode(const std::string& encoded);

}
