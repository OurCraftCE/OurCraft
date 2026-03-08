#include "CryptoUtils.h"
#include <monocypher.h>
#include <fstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace CryptoUtils {

void randomBytes(uint8_t* buf, size_t len) {
#ifdef _WIN32
    BCryptGenRandom(NULL, buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#else
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    urandom.read(reinterpret_cast<char*>(buf), len);
#endif
}

Ed25519KeyPair generateSigningKeyPair() {
    Ed25519KeyPair kp;
    uint8_t seed[32];
    randomBytes(seed, sizeof(seed));
    crypto_eddsa_key_pair(kp.privateKey, kp.publicKey, seed);
    crypto_wipe(seed, sizeof(seed));
    return kp;
}

std::vector<uint8_t> sign(const uint8_t* message, size_t len, const uint8_t privateKey[64]) {
    std::vector<uint8_t> sig(64);
    crypto_eddsa_sign(sig.data(), privateKey, message, len);
    return sig;
}

bool verify(const uint8_t* message, size_t len, const uint8_t signature[64], const uint8_t publicKey[32]) {
    return crypto_eddsa_check(signature, publicKey, message, len) == 0;
}

X25519KeyPair generateX25519KeyPair() {
    X25519KeyPair kp;
    randomBytes(kp.secretKey, sizeof(kp.secretKey));
    crypto_x25519_public_key(kp.publicKey, kp.secretKey);
    return kp;
}

std::vector<uint8_t> x25519SharedSecret(const uint8_t ourSecret[32], const uint8_t theirPublic[32]) {
    std::vector<uint8_t> shared(32);
    crypto_x25519(shared.data(), ourSecret, theirPublic);
    return shared;
}

std::vector<uint8_t> encrypt(const uint8_t* plaintext, size_t len, const uint8_t key[32], const uint8_t nonce[24]) {
    // Output format: MAC (16 bytes) + ciphertext
    std::vector<uint8_t> output(16 + len);
    uint8_t* mac = output.data();
    uint8_t* cipher = output.data() + 16;
    crypto_aead_lock(cipher, mac, key, nonce, nullptr, 0, plaintext, len);
    return output;
}

std::vector<uint8_t> decrypt(const uint8_t* ciphertext, size_t len, const uint8_t key[32], const uint8_t nonce[24]) {
    // Input format: MAC (16 bytes) + ciphertext
    if (len < 16) {
        return {};
    }
    const uint8_t* mac = ciphertext;
    const uint8_t* cipher = ciphertext + 16;
    size_t plainLen = len - 16;
    std::vector<uint8_t> plain(plainLen);
    int result = crypto_aead_unlock(plain.data(), mac, key, nonce, nullptr, 0, cipher, plainLen);
    if (result != 0) {
        return {};
    }
    return plain;
}

std::vector<uint8_t> hash256(const uint8_t* data, size_t len) {
    std::vector<uint8_t> hash(32);
    crypto_blake2b(hash.data(), 32, data, len);
    return hash;
}

bool saveKeyFile(const char* path, const uint8_t* data, size_t len) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data), len);
    return file.good();
}

std::vector<uint8_t> loadKeyFile(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);
    if (!file.good()) {
        return {};
    }
    return data;
}

static const char base64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const uint8_t* data, size_t len) {
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t octet_a = data[i];
        uint32_t octet_b = (i + 1 < len) ? data[i + 1] : 0;
        uint32_t octet_c = (i + 2 < len) ? data[i + 2] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        result += base64Chars[(triple >> 18) & 0x3F];
        result += base64Chars[(triple >> 12) & 0x3F];
        result += (i + 1 < len) ? base64Chars[(triple >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? base64Chars[triple & 0x3F] : '=';
    }
    return result;
}

std::vector<uint8_t> base64Decode(const std::string& encoded) {
    static const uint8_t decodeTable[128] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
        52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64
    };

    std::vector<uint8_t> result;
    if (encoded.empty()) return result;

    size_t padding = 0;
    if (encoded.size() >= 1 && encoded[encoded.size() - 1] == '=') padding++;
    if (encoded.size() >= 2 && encoded[encoded.size() - 2] == '=') padding++;

    result.reserve((encoded.size() / 4) * 3 - padding);

    for (size_t i = 0; i < encoded.size(); i += 4) {
        uint32_t a = (i     < encoded.size() && encoded[i]     != '=') ? decodeTable[(uint8_t)encoded[i]]     : 0;
        uint32_t b = (i + 1 < encoded.size() && encoded[i + 1] != '=') ? decodeTable[(uint8_t)encoded[i + 1]] : 0;
        uint32_t c = (i + 2 < encoded.size() && encoded[i + 2] != '=') ? decodeTable[(uint8_t)encoded[i + 2]] : 0;
        uint32_t d = (i + 3 < encoded.size() && encoded[i + 3] != '=') ? decodeTable[(uint8_t)encoded[i + 3]] : 0;

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

        result.push_back((triple >> 16) & 0xFF);
        if (i + 2 < encoded.size() && encoded[i + 2] != '=')
            result.push_back((triple >> 8) & 0xFF);
        if (i + 3 < encoded.size() && encoded[i + 3] != '=')
            result.push_back(triple & 0xFF);
    }
    return result;
}

} // namespace CryptoUtils
