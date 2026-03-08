#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace ModTransfer {

constexpr uint8_t PACKET_MOD_TRANSFER = 0xFC;

enum class TransferSubType : uint8_t {
    ModManifest       = 0x01,
    ResourceManifest  = 0x02,
    FileChunk         = 0x03,
    TransferComplete  = 0x04,
    ClientResponse    = 0x05
};

struct ModManifestEntry {
    std::string modId;
    std::string modName;
    std::string version;
    uint32_t    fileSize;
    uint8_t     signature[64];
    uint8_t     fileHash[32];
};

struct ResourceManifestEntry {
    std::string uuid;
    std::string name;
    std::string version;
    uint32_t    fileSize;
    uint8_t     fileHash[32];
};

struct ModManifest {
    uint8_t serverPublicKey[32];
    uint8_t x25519PublicKey[32];
    std::vector<ModManifestEntry> entries;
};

struct ResourceManifest {
    bool forceServerPacks;
    std::vector<ResourceManifestEntry> entries;
};

struct FileChunk {
    uint8_t  fileHash[32];
    uint32_t offset;
    uint32_t totalSize;
    std::vector<uint8_t> data;
};

enum class ClientResponseType : uint8_t {
    AcceptMods  = 1,
    DeclineMods = 2,
    Ready       = 3
};

struct ClientResponse {
    ClientResponseType type;
    uint8_t x25519PublicKey[32];
};

namespace ModTransferCodec {

std::vector<uint8_t> encode(const ModManifest& manifest);
std::vector<uint8_t> encode(const ResourceManifest& manifest);
std::vector<uint8_t> encode(const FileChunk& chunk);
std::vector<uint8_t> encode(const ClientResponse& response);

bool decode(const uint8_t* data, size_t len, ModManifest& out);
bool decode(const uint8_t* data, size_t len, ResourceManifest& out);
bool decode(const uint8_t* data, size_t len, FileChunk& out);
bool decode(const uint8_t* data, size_t len, ClientResponse& out);

} // namespace ModTransferCodec
} // namespace ModTransfer
