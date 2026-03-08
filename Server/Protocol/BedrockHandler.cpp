#include "BedrockHandler.h"
#include "ModTransfer.h"
#include "..\ModDistributor.h"
#include "../../Core/Network/RakNetTransport.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <ctime>

const char* BedrockHandler::VERSION_NAME = "1.12.0";

static std::string toNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    return std::string(w.begin(), w.end());
}

BedrockHandler::BedrockHandler() {
    srand((unsigned int)time(nullptr));
    m_serverGuid = ((uint64_t)rand() << 32) | (uint64_t)rand();
}

BedrockHandler::~BedrockHandler() {
}

void BedrockHandler::init(RakNetTransport* transport, uint16_t port) {
    m_transport = transport;
    m_port = port;
}

void BedrockHandler::updateMotd(const std::wstring& motd, int onlinePlayers, int maxPlayers,
                                 const std::wstring& worldName, const std::wstring& gamemode) {
    // Bedrock unconnected-pong string: MCPE;motd;protocol;version;online;max;guid;world;gamemode;1;port;port;
    std::ostringstream ss;
    ss << "MCPE;"
       << toNarrow(motd) << ";"
       << PROTOCOL_VERSION << ";"
       << VERSION_NAME << ";"
       << onlinePlayers << ";"
       << maxPlayers << ";"
       << m_serverGuid << ";"
       << toNarrow(worldName) << ";"
       << toNarrow(gamemode) << ";"
       << "1;"
       << m_port << ";"
       << m_port << ";";

    m_motdString = ss.str();

    m_transport->setOfflinePingResponse(m_motdString.c_str(), (unsigned int)m_motdString.size());
    printf("[Bedrock] MOTD set: %s\n", toNarrow(motd).c_str());
}

void BedrockHandler::tick() {
    if (!m_transport) return;
    // Bedrock login packet processing will go here; connect/disconnect handled in ServerMain::tick()
}

void BedrockHandler::sendGamePacket(uint64_t clientId, const uint8_t* data, uint32_t len) {
    if (!m_transport) return;
    ConnectionId conn;
    conn.id = clientId;
    std::vector<uint8_t> wrapped; // 0xFE game packet wrapper byte
    wrapped.reserve(1 + len);
    wrapped.push_back(0xFE);
    wrapped.insert(wrapped.end(), data, data + len);
    m_transport->send(conn, wrapped.data(), (uint32_t)wrapped.size(),
                      PacketPriority::High, PacketReliability::ReliableOrdered);
}

void BedrockHandler::sendModManifest(uint64_t clientId) {
    auto& dist = ModDistributor::instance();
    if (!dist.hasMods() && !dist.hasPacks()) return;

    auto encoded = ModTransfer::ModTransferCodec::encode(dist.getModManifest());

    std::vector<uint8_t> payload;
    payload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
    payload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::ModManifest));
    payload.insert(payload.end(), encoded.begin(), encoded.end());

    sendGamePacket(clientId, payload.data(), (uint32_t)payload.size());
    printf("[Bedrock] Sent mod manifest to %llu (%zu bytes)\n", clientId, encoded.size());
}

void BedrockHandler::sendResourceManifest(uint64_t clientId) {
    auto& dist = ModDistributor::instance();
    if (!dist.hasPacks() && !dist.hasMods()) return;

    auto encoded = ModTransfer::ModTransferCodec::encode(dist.getResourceManifest());

    std::vector<uint8_t> payload;
    payload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
    payload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::ResourceManifest));
    payload.insert(payload.end(), encoded.begin(), encoded.end());

    sendGamePacket(clientId, payload.data(), (uint32_t)payload.size());
    printf("[Bedrock] Sent resource manifest to %llu (%zu bytes)\n", clientId, encoded.size());
}

void BedrockHandler::sendFileChunks(uint64_t clientId, const uint8_t clientX25519Key[32]) {
    auto& dist = ModDistributor::instance();
    const uint32_t CHUNK_SIZE = 65536;

    for (const auto& mod : dist.getMods()) {
        auto encrypted = dist.encryptForClient(mod, clientX25519Key);
        uint32_t offset = 0;
        while (offset < (uint32_t)encrypted.size()) {
            uint32_t chunkLen = CHUNK_SIZE;
            if (offset + chunkLen > (uint32_t)encrypted.size())
                chunkLen = (uint32_t)encrypted.size() - offset;

            ModTransfer::FileChunk chunk = {};
            memcpy(chunk.fileHash, mod.fileHash, 32);
            chunk.offset = offset;
            chunk.totalSize = (uint32_t)encrypted.size();
            chunk.data.assign(encrypted.data() + offset, encrypted.data() + offset + chunkLen);

            auto encoded = ModTransfer::ModTransferCodec::encode(chunk);
            std::vector<uint8_t> payload;
            payload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
            payload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::FileChunk));
            payload.insert(payload.end(), encoded.begin(), encoded.end());
            sendGamePacket(clientId, payload.data(), (uint32_t)payload.size());

            offset += chunkLen;
        }

        std::vector<uint8_t> completePayload;
        completePayload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
        completePayload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::TransferComplete));
        completePayload.insert(completePayload.end(), mod.fileHash, mod.fileHash + 32);
        sendGamePacket(clientId, completePayload.data(), (uint32_t)completePayload.size());

        printf("[Bedrock] Sent mod %s to %llu (%zu bytes encrypted)\n",
               mod.modId.c_str(), clientId, encrypted.size());
    }

    for (const auto& pack : dist.getPacks()) {
        const auto& data = pack.fileData;
        uint32_t offset = 0;
        while (offset < (uint32_t)data.size()) {
            uint32_t chunkLen = CHUNK_SIZE;
            if (offset + chunkLen > (uint32_t)data.size())
                chunkLen = (uint32_t)data.size() - offset;

            ModTransfer::FileChunk chunk = {};
            memcpy(chunk.fileHash, pack.fileHash, 32);
            chunk.offset = offset;
            chunk.totalSize = (uint32_t)data.size();
            chunk.data.assign(data.data() + offset, data.data() + offset + chunkLen);

            auto encoded = ModTransfer::ModTransferCodec::encode(chunk);
            std::vector<uint8_t> payload;
            payload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
            payload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::FileChunk));
            payload.insert(payload.end(), encoded.begin(), encoded.end());
            sendGamePacket(clientId, payload.data(), (uint32_t)payload.size());

            offset += chunkLen;
        }

        std::vector<uint8_t> completePayload;
        completePayload.push_back(ModTransfer::PACKET_MOD_TRANSFER);
        completePayload.push_back(static_cast<uint8_t>(ModTransfer::TransferSubType::TransferComplete));
        completePayload.insert(completePayload.end(), pack.fileHash, pack.fileHash + 32);
        sendGamePacket(clientId, completePayload.data(), (uint32_t)completePayload.size());

        printf("[Bedrock] Sent pack %s to %llu (%u bytes)\n",
               pack.name.c_str(), clientId, pack.fileSize);
    }
}

void BedrockHandler::handleTransferPacket(uint64_t clientId, const uint8_t* data, size_t len) {
    if (len < 1) return;

    ModTransfer::ClientResponse resp = {};
    if (ModTransfer::ModTransferCodec::decode(data, len, resp)) {
        switch (resp.type) {
            case ModTransfer::ClientResponseType::AcceptMods:
                printf("[Bedrock] Client %llu accepted mods\n", clientId);
                sendFileChunks(clientId, resp.x25519PublicKey);
                break;
            case ModTransfer::ClientResponseType::DeclineMods:
                printf("[Bedrock] Client %llu declined mods\n", clientId);
                break;
            case ModTransfer::ClientResponseType::Ready:
                printf("[Bedrock] Client %llu transfer complete\n", clientId);
                break;
        }
    }
}
