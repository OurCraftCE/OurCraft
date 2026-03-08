#include "ModTransfer.h"
#include <cstring>

namespace ModTransfer {
namespace ModTransferCodec {

static void writeU8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

static void writeU16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void writeU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void writeString(std::vector<uint8_t>& buf, const std::string& s) {
    writeU16(buf, static_cast<uint16_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

static void writeBytes(std::vector<uint8_t>& buf, const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

struct Reader {
    const uint8_t* data;
    size_t         len;
    size_t         pos;

    Reader(const uint8_t* d, size_t l) : data(d), len(l), pos(0) {}

    bool hasRemaining(size_t n) const { return pos + n <= len; }

    bool readU8(uint8_t& v) {
        if (!hasRemaining(1)) return false;
        v = data[pos++];
        return true;
    }

    bool readU16(uint16_t& v) {
        if (!hasRemaining(2)) return false;
        v = static_cast<uint16_t>((data[pos] << 8) | data[pos + 1]);
        pos += 2;
        return true;
    }

    bool readU32(uint32_t& v) {
        if (!hasRemaining(4)) return false;
        v = (static_cast<uint32_t>(data[pos]) << 24) |
            (static_cast<uint32_t>(data[pos + 1]) << 16) |
            (static_cast<uint32_t>(data[pos + 2]) << 8) |
            static_cast<uint32_t>(data[pos + 3]);
        pos += 4;
        return true;
    }

    bool readString(std::string& s) {
        uint16_t slen;
        if (!readU16(slen)) return false;
        if (!hasRemaining(slen)) return false;
        s.assign(reinterpret_cast<const char*>(data + pos), slen);
        pos += slen;
        return true;
    }

    bool readBytes(uint8_t* out, size_t n) {
        if (!hasRemaining(n)) return false;
        std::memcpy(out, data + pos, n);
        pos += n;
        return true;
    }

    bool readBytes(std::vector<uint8_t>& out, size_t n) {
        if (!hasRemaining(n)) return false;
        out.assign(data + pos, data + pos + n);
        pos += n;
        return true;
    }
};

std::vector<uint8_t> encode(const ModManifest& manifest) {
    std::vector<uint8_t> buf;
    writeU8(buf, PACKET_MOD_TRANSFER);
    writeU8(buf, static_cast<uint8_t>(TransferSubType::ModManifest));
    writeBytes(buf, manifest.serverPublicKey, 32);
    writeBytes(buf, manifest.x25519PublicKey, 32);
    writeU16(buf, static_cast<uint16_t>(manifest.entries.size()));
    for (auto& e : manifest.entries) {
        writeString(buf, e.modId);
        writeString(buf, e.modName);
        writeString(buf, e.version);
        writeU32(buf, e.fileSize);
        writeBytes(buf, e.signature, 64);
        writeBytes(buf, e.fileHash, 32);
    }
    return buf;
}

std::vector<uint8_t> encode(const ResourceManifest& manifest) {
    std::vector<uint8_t> buf;
    writeU8(buf, PACKET_MOD_TRANSFER);
    writeU8(buf, static_cast<uint8_t>(TransferSubType::ResourceManifest));
    writeU8(buf, manifest.forceServerPacks ? 1 : 0);
    writeU16(buf, static_cast<uint16_t>(manifest.entries.size()));
    for (auto& e : manifest.entries) {
        writeString(buf, e.uuid);
        writeString(buf, e.name);
        writeString(buf, e.version);
        writeU32(buf, e.fileSize);
        writeBytes(buf, e.fileHash, 32);
    }
    return buf;
}

std::vector<uint8_t> encode(const FileChunk& chunk) {
    std::vector<uint8_t> buf;
    writeU8(buf, PACKET_MOD_TRANSFER);
    writeU8(buf, static_cast<uint8_t>(TransferSubType::FileChunk));
    writeBytes(buf, chunk.fileHash, 32);
    writeU32(buf, chunk.offset);
    writeU32(buf, chunk.totalSize);
    writeU32(buf, static_cast<uint32_t>(chunk.data.size()));
    writeBytes(buf, chunk.data.data(), chunk.data.size());
    return buf;
}

std::vector<uint8_t> encode(const ClientResponse& response) {
    std::vector<uint8_t> buf;
    writeU8(buf, PACKET_MOD_TRANSFER);
    writeU8(buf, static_cast<uint8_t>(TransferSubType::ClientResponse));
    writeU8(buf, static_cast<uint8_t>(response.type));
    writeBytes(buf, response.x25519PublicKey, 32);
    return buf;
}

bool decode(const uint8_t* data, size_t len, ModManifest& out) {
    Reader r(data, len);
    uint8_t packetId, subType;
    if (!r.readU8(packetId) || packetId != PACKET_MOD_TRANSFER) return false;
    if (!r.readU8(subType) || subType != static_cast<uint8_t>(TransferSubType::ModManifest)) return false;
    if (!r.readBytes(out.serverPublicKey, 32)) return false;
    if (!r.readBytes(out.x25519PublicKey, 32)) return false;
    uint16_t count;
    if (!r.readU16(count)) return false;
    out.entries.resize(count);
    for (uint16_t i = 0; i < count; i++) {
        auto& e = out.entries[i];
        if (!r.readString(e.modId)) return false;
        if (!r.readString(e.modName)) return false;
        if (!r.readString(e.version)) return false;
        if (!r.readU32(e.fileSize)) return false;
        if (!r.readBytes(e.signature, 64)) return false;
        if (!r.readBytes(e.fileHash, 32)) return false;
    }
    return true;
}

bool decode(const uint8_t* data, size_t len, ResourceManifest& out) {
    Reader r(data, len);
    uint8_t packetId, subType;
    if (!r.readU8(packetId) || packetId != PACKET_MOD_TRANSFER) return false;
    if (!r.readU8(subType) || subType != static_cast<uint8_t>(TransferSubType::ResourceManifest)) return false;
    uint8_t force;
    if (!r.readU8(force)) return false;
    out.forceServerPacks = (force != 0);
    uint16_t count;
    if (!r.readU16(count)) return false;
    out.entries.resize(count);
    for (uint16_t i = 0; i < count; i++) {
        auto& e = out.entries[i];
        if (!r.readString(e.uuid)) return false;
        if (!r.readString(e.name)) return false;
        if (!r.readString(e.version)) return false;
        if (!r.readU32(e.fileSize)) return false;
        if (!r.readBytes(e.fileHash, 32)) return false;
    }
    return true;
}

bool decode(const uint8_t* data, size_t len, FileChunk& out) {
    Reader r(data, len);
    uint8_t packetId, subType;
    if (!r.readU8(packetId) || packetId != PACKET_MOD_TRANSFER) return false;
    if (!r.readU8(subType) || subType != static_cast<uint8_t>(TransferSubType::FileChunk)) return false;
    if (!r.readBytes(out.fileHash, 32)) return false;
    if (!r.readU32(out.offset)) return false;
    if (!r.readU32(out.totalSize)) return false;
    uint32_t dataLen;
    if (!r.readU32(dataLen)) return false;
    if (!r.readBytes(out.data, dataLen)) return false;
    return true;
}

bool decode(const uint8_t* data, size_t len, ClientResponse& out) {
    Reader r(data, len);
    uint8_t packetId, subType;
    if (!r.readU8(packetId) || packetId != PACKET_MOD_TRANSFER) return false;
    if (!r.readU8(subType) || subType != static_cast<uint8_t>(TransferSubType::ClientResponse)) return false;
    uint8_t type;
    if (!r.readU8(type)) return false;
    out.type = static_cast<ClientResponseType>(type);
    if (!r.readBytes(out.x25519PublicKey, 32)) return false;
    return true;
}

} // namespace ModTransferCodec
} // namespace ModTransfer
