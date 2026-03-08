#include "JavaListener.h"
#include "VarInt.h"
#include "ModTransfer.h"
#include "..\ModDistributor.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <wincrypt.h>
#include <winhttp.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "winhttp.lib")

const char* JavaListener::VERSION_NAME = "1.8.9";

// FNV-1a pseudo-MD5 for offline UUID v3 (deterministic, not cryptographic)
static void simpleMd5(const uint8_t* data, size_t len, uint8_t out[16]) {
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0x100000001b3ULL;
    for (size_t i = 0; i < len; i++) {
        h1 ^= data[i]; h1 *= 0x01000193;
        h2 ^= data[i]; h2 *= 0x01000193;
    }
    memcpy(out, &h1, 8);
    memcpy(out + 8, &h2, 8);
    out[6] = (out[6] & 0x0F) | 0x30; // version 3
    out[8] = (out[8] & 0x3F) | 0x80; // variant bits
}

static std::string toNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    return std::string(w.begin(), w.end());
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c;
        }
    }
    return out;
}

static void writeVarInt(std::vector<uint8_t>& buf, int32_t val);
static void writeString(std::vector<uint8_t>& buf, const std::string& s);

JavaListener::JavaListener() {}

JavaListener::~JavaListener() {
    stop();
    cleanupCrypto();
}

bool JavaListener::initCrypto() {
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextA(&hProv, nullptr, MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES, CRYPT_NEWKEYSET)) {
        if (GetLastError() == NTE_EXISTS) {
            if (!CryptAcquireContextA(&hProv, nullptr, MS_ENH_RSA_AES_PROV_A, PROV_RSA_AES, 0))
                return false;
        } else {
            return false;
        }
    }
    m_cryptProv = (void*)(uintptr_t)hProv;

    HCRYPTKEY hKey = 0;
    if (!CryptGenKey(hProv, AT_KEYEXCHANGE, (1024 << 16) | CRYPT_EXPORTABLE, &hKey)) {
        CryptReleaseContext(hProv, 0);
        m_cryptProv = nullptr;
        return false;
    }
    m_rsaKey = (void*)(uintptr_t)hKey;

    // export public key as DER (X.509 SubjectPublicKeyInfo)
    DWORD derLen = 0;
    CryptExportPublicKeyInfo(hProv, AT_KEYEXCHANGE, X509_ASN_ENCODING, nullptr, &derLen);
    std::vector<uint8_t> pubKeyInfo(derLen);
    CryptExportPublicKeyInfo(hProv, AT_KEYEXCHANGE, X509_ASN_ENCODING,
                              (CERT_PUBLIC_KEY_INFO*)pubKeyInfo.data(), &derLen);

    DWORD encodedLen = 0;
    CryptEncodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO,
                         pubKeyInfo.data(), 0, nullptr, nullptr, &encodedLen);
    m_publicKeyDer.resize(encodedLen);
    CryptEncodeObjectEx(X509_ASN_ENCODING, X509_PUBLIC_KEY_INFO,
                         pubKeyInfo.data(), 0, nullptr, m_publicKeyDer.data(), &encodedLen);
    m_publicKeyDer.resize(encodedLen);

    printf("[Java] RSA 1024-bit keypair generated for online mode\n");
    return true;
}

void JavaListener::cleanupCrypto() {
    if (m_rsaKey) {
        CryptDestroyKey((HCRYPTKEY)(uintptr_t)m_rsaKey);
        m_rsaKey = nullptr;
    }
    if (m_cryptProv) {
        CryptReleaseContext((HCRYPTPROV)(uintptr_t)m_cryptProv, 0);
        m_cryptProv = nullptr;
    }
    m_publicKeyDer.clear();
}

bool JavaListener::start(uint16_t port, int maxPlayers) {
    m_port = port;
    m_maxPlayers = maxPlayers;

    if (m_onlineMode) {
        if (!initCrypto()) {
            printf("[Java] WARNING: Failed to init RSA, falling back to offline mode\n");
            m_onlineMode = false;
        }
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSock == INVALID_SOCKET) {
        printf("[Java] ERROR: Failed to create TCP socket\n");
        return false;
    }

    int opt = 1;
    setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(m_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Java] ERROR: Failed to bind TCP port %u\n", port);
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
        return false;
    }

    if (::listen(m_listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("[Java] ERROR: Failed to listen on TCP port %u\n", port);
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_acceptThread = CreateThread(nullptr, 0, acceptThread, this, 0, nullptr);
    return true;
}

void JavaListener::stop() {
    m_running = false;

    if (m_listenSock != INVALID_SOCKET) {
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
    }

    if (m_acceptThread) {
        WaitForSingleObject(m_acceptThread, 5000);
        CloseHandle(m_acceptThread);
        m_acceptThread = nullptr;
    }

    std::lock_guard<std::mutex> lock(m_clientsMtx);
    for (auto& c : m_clients) {
        if (c.sock != INVALID_SOCKET) closesocket(c.sock);
    }
    m_clients.clear();
}

DWORD WINAPI JavaListener::acceptThread(LPVOID param) {
    static_cast<JavaListener*>(param)->acceptLoop();
    return 0;
}

void JavaListener::acceptLoop() {
    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_listenSock, &readSet);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int sel = select(0, &readSet, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(m_listenSock, (sockaddr*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET) continue;

        u_long nonBlocking = 1;
        ioctlsocket(clientSock, FIONBIO, &nonBlocking);

        int nodelay = 1;
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));

        JavaClient client;
        client.sock = clientSock;
        client.state = ClientState::Handshake;
        client.connectionId = m_nextConnId++;
        client.lastActivity = GetTickCount();

        printf("[Java] New connection from %s:%d (id=%llu)\n",
               addrStr, ntohs(clientAddr.sin_port), client.connectionId);

        std::lock_guard<std::mutex> lock(m_clientsMtx);
        m_clients.push_back(std::move(client));
    }
}

void JavaListener::tick() {
    std::lock_guard<std::mutex> lock(m_clientsMtx);

    DWORD now = GetTickCount();

    for (auto it = m_clients.begin(); it != m_clients.end(); ) {
        JavaClient& client = *it;

        uint8_t buf[4096];
        int received = recv(client.sock, (char*)buf, sizeof(buf), 0);
        if (received > 0) {
            client.recvBuf.insert(client.recvBuf.end(), buf, buf + received);
            client.lastActivity = now;
        } else if (received == 0) {
            client.state = ClientState::Disconnected;
        } else {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                client.state = ClientState::Disconnected;
            }
        }

        if (now - client.lastActivity > CLIENT_TIMEOUT_MS) {
            client.state = ClientState::Disconnected;
        }

        if (client.state != ClientState::Disconnected) {
            processClient(client);
        }

        if (client.state == ClientState::Play && (now - client.lastKeepAlive) > 10000) {
            sendKeepAlive(client);
        }

        if (client.state == ClientState::Disconnected) {
            if (!client.username.empty()) {
                printf("[Java] Player %s disconnected\n", client.username.c_str());
            }
            closesocket(client.sock);
            it = m_clients.erase(it);
        } else {
            ++it;
        }
    }
}

bool JavaListener::readPacket(JavaClient& client, int32_t& packetId, std::vector<uint8_t>& payload) {
    if (client.recvBuf.size() < 2) return false;

    int32_t packetLen = 0;
    int lenBytes = VarInt::read(client.recvBuf.data(), (int)client.recvBuf.size(), &packetLen);
    if (lenBytes == 0) return false;
    if (packetLen <= 0 || packetLen > 1048576) { // 1MB max
        client.state = ClientState::Disconnected;
        return false;
    }
    if ((int)client.recvBuf.size() < lenBytes + packetLen) return false;

    const uint8_t* pktData = client.recvBuf.data() + lenBytes;
    int32_t pktId = 0;
    int idBytes = VarInt::read(pktData, packetLen, &pktId);
    if (idBytes == 0) {
        client.state = ClientState::Disconnected;
        return false;
    }

    packetId = pktId;
    payload.assign(pktData + idBytes, pktData + packetLen);

    client.recvBuf.erase(client.recvBuf.begin(), client.recvBuf.begin() + lenBytes + packetLen);
    return true;
}

void JavaListener::processClient(JavaClient& client) {
    int32_t packetId;
    std::vector<uint8_t> payload;

    while (readPacket(client, packetId, payload)) {
        switch (client.state) {
        case ClientState::Handshake:
            if (packetId == 0x00) handleHandshake(client, payload);
            break;
        case ClientState::Status:
            if (packetId == 0x00) handleStatusRequest(client);
            else if (packetId == 0x01) handleStatusPing(client, payload);
            break;
        case ClientState::Login:
            if (packetId == 0x00) handleLoginStart(client, payload);
            else if (packetId == 0x01) handleEncryptionResponse(client, payload);
            break;
        case ClientState::Play:
            if (packetId == 0x00) handleKeepAliveResponse(client, payload);
            else if (packetId == ModTransfer::PACKET_MOD_TRANSFER) handleModTransferResponse(client, payload);
                break;
        default:
            break;
        }

        if (client.state == ClientState::Disconnected) break;
    }
}

void JavaListener::handleHandshake(JavaClient& client, const std::vector<uint8_t>& payload) {
    const uint8_t* p = payload.data();
    int rem = (int)payload.size();

    int32_t protocolVersion = 0;
    int n = VarInt::read(p, rem, &protocolVersion);
    if (n == 0) return;
    p += n; rem -= n;

    // server address: VarInt length + UTF-8
    int32_t addrLen = 0;
    n = VarInt::read(p, rem, &addrLen);
    if (n == 0) return;
    p += n; rem -= n;
    if (addrLen > rem) return;
    p += addrLen; rem -= addrLen;

    if (rem < 2) return; // skip big-endian port
    p += 2; rem -= 2;

    // next state: 1=Status, 2=Login
    int32_t nextState = 0;
    n = VarInt::read(p, rem, &nextState);
    if (n == 0) return;

    if (nextState == 1) {
        client.state = ClientState::Status;
    } else if (nextState == 2) {
        client.state = ClientState::Login;
    } else {
        client.state = ClientState::Disconnected;
    }
}

void JavaListener::handleStatusRequest(JavaClient& client) {
    std::string json = buildStatusJson();

    std::vector<uint8_t> data;
    uint8_t lenBuf[5];
    int lenBytes = VarInt::write(lenBuf, (int32_t)json.size());
    data.insert(data.end(), lenBuf, lenBuf + lenBytes);
    data.insert(data.end(), json.begin(), json.end());

    sendPacket(client, 0x00, data);
}

void JavaListener::handleStatusPing(JavaClient& client, const std::vector<uint8_t>& payload) {
    sendPacket(client, 0x01, payload);
}

void JavaListener::handleLoginStart(JavaClient& client, const std::vector<uint8_t>& payload) {
    const uint8_t* p = payload.data();
    int rem = (int)payload.size();

    int32_t nameLen = 0;
    int n = VarInt::read(p, rem, &nameLen);
    if (n == 0 || nameLen <= 0 || nameLen > 16) {
        client.state = ClientState::Disconnected;
        return;
    }
    p += n; rem -= n;
    if (nameLen > rem) {
        client.state = ClientState::Disconnected;
        return;
    }

    client.username.assign((const char*)p, nameLen);
    printf("[Java] Player login: %s (online-mode=%s)\n",
           client.username.c_str(), m_onlineMode ? "true" : "false");

    if (m_onlineMode && m_rsaKey) {
        sendEncryptionRequest(client);
    } else {
        std::string uuid = offlineUuid(client.username);
        completeLogin(client, uuid, client.username);
    }
}

void JavaListener::sendEncryptionRequest(JavaClient& client) {
    client.verifyToken.resize(4);
    HCRYPTPROV hProv = (HCRYPTPROV)(uintptr_t)m_cryptProv;
    CryptGenRandom(hProv, 4, client.verifyToken.data());

    std::vector<uint8_t> data;
    writeString(data, ""); // empty server ID for 1.8+
    writeVarInt(data, (int32_t)m_publicKeyDer.size());
    data.insert(data.end(), m_publicKeyDer.begin(), m_publicKeyDer.end());
    writeVarInt(data, (int32_t)client.verifyToken.size());
    data.insert(data.end(), client.verifyToken.begin(), client.verifyToken.end());

    sendPacket(client, 0x01, data);
}

void JavaListener::handleEncryptionResponse(JavaClient& client, const std::vector<uint8_t>& payload) {
    if (!m_rsaKey) {
        client.state = ClientState::Disconnected;
        return;
    }

    const uint8_t* p = payload.data();
    int rem = (int)payload.size();

    int32_t secretLen = 0;
    int n = VarInt::read(p, rem, &secretLen);
    if (n == 0 || secretLen <= 0 || secretLen > 256) { client.state = ClientState::Disconnected; return; }
    p += n; rem -= n;
    if (secretLen > rem) { client.state = ClientState::Disconnected; return; }
    std::vector<uint8_t> encSecret(p, p + secretLen);
    p += secretLen; rem -= secretLen;

    int32_t tokenLen = 0;
    n = VarInt::read(p, rem, &tokenLen);
    if (n == 0 || tokenLen <= 0 || tokenLen > 256) { client.state = ClientState::Disconnected; return; }
    p += n; rem -= n;
    if (tokenLen > rem) { client.state = ClientState::Disconnected; return; }
    std::vector<uint8_t> encToken(p, p + tokenLen);

    HCRYPTKEY hKey = (HCRYPTKEY)(uintptr_t)m_rsaKey;

    // CryptoAPI expects little-endian; Java sends big-endian
    std::reverse(encSecret.begin(), encSecret.end());
    DWORD decSecretLen = (DWORD)encSecret.size();
    if (!CryptDecrypt(hKey, 0, TRUE, 0, encSecret.data(), &decSecretLen)) {
        printf("[Java] Failed to decrypt shared secret (error=%lu)\n", GetLastError());
        sendDisconnect(client, "Encryption failed");
        return;
    }
    client.sharedSecret.assign(encSecret.begin(), encSecret.begin() + decSecretLen);

    std::reverse(encToken.begin(), encToken.end());
    DWORD decTokenLen = (DWORD)encToken.size();
    if (!CryptDecrypt(hKey, 0, TRUE, 0, encToken.data(), &decTokenLen)) {
        printf("[Java] Failed to decrypt verify token\n");
        sendDisconnect(client, "Encryption failed");
        return;
    }

    if (decTokenLen != client.verifyToken.size() ||
        memcmp(encToken.data(), client.verifyToken.data(), decTokenLen) != 0) {
        printf("[Java] Verify token mismatch for %s\n", client.username.c_str());
        sendDisconnect(client, "Invalid verify token");
        return;
    }

    std::string serverHash = computeServerHash("", client.sharedSecret, m_publicKeyDer);
    std::string realUuid, realName;
    if (verifyMojangSession(client.username, serverHash, realUuid, realName)) {
        printf("[Java] Mojang auth OK for %s (uuid=%s)\n", realName.c_str(), realUuid.c_str());
        client.authenticated = true;
        completeLogin(client, realUuid, realName);
    } else {
        printf("[Java] Mojang auth FAILED for %s\n", client.username.c_str());
        sendDisconnect(client, "Failed to verify username!");
    }
}

void JavaListener::completeLogin(JavaClient& client, const std::string& uuid, const std::string& name) {
    client.uuid = uuid;
    client.username = name;

    std::vector<uint8_t> data;
    writeString(data, uuid);
    writeString(data, name);
    sendPacket(client, 0x02, data);

    printf("[Java] Player %s logged in (uuid=%s, auth=%s)\n",
           name.c_str(), uuid.c_str(),
           client.authenticated ? "mojang" : "offline");
    client.state = ClientState::Play;

    auto& dist = ModDistributor::instance();
    if (dist.hasMods() || dist.hasPacks()) {
        client.inTransferPhase = true;
        client.transferComplete = false;
        sendModManifest(client);
        sendResourceManifest(client);
        // defer JoinGame until client signals Ready
    } else {
        sendJoinGame(client);
    }
}

// Minecraft-style signed SHA-1 hex (two's complement if MSB set)
std::string JavaListener::computeServerHash(const std::string& serverId,
                                             const std::vector<uint8_t>& sharedSecret,
                                             const std::vector<uint8_t>& publicKey) {
    HCRYPTPROV hProv = (HCRYPTPROV)(uintptr_t)m_cryptProv;
    HCRYPTHASH hHash = 0;
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);

    // hash order: serverId + sharedSecret + publicKey
    if (!serverId.empty())
        CryptHashData(hHash, (const BYTE*)serverId.c_str(), (DWORD)serverId.size(), 0);
    CryptHashData(hHash, sharedSecret.data(), (DWORD)sharedSecret.size(), 0);
    CryptHashData(hHash, publicKey.data(), (DWORD)publicKey.size(), 0);

    BYTE sha1[20];
    DWORD sha1Len = 20;
    CryptGetHashParam(hHash, HP_HASHVAL, sha1, &sha1Len, 0);
    CryptDestroyHash(hHash);

    bool negative = (sha1[0] & 0x80) != 0;
    if (negative) {
        // two's complement negation
        bool carry = true;
        for (int i = 19; i >= 0; i--) {
            sha1[i] = ~sha1[i];
            if (carry) {
                if (sha1[i] == 0xFF) { sha1[i] = 0; carry = true; }
                else { sha1[i]++; carry = false; }
            }
        }
    }

    char hex[41];
    for (int i = 0; i < 20; i++)
        snprintf(hex + i * 2, 3, "%02x", sha1[i]);
    hex[40] = '\0';

    const char* start = hex;
    while (*start == '0' && *(start + 1) != '\0') start++;

    std::string result;
    if (negative) result = "-";
    result += start;
    return result;
}

bool JavaListener::verifyMojangSession(const std::string& username, const std::string& serverHash,
                                        std::string& outUuid, std::string& outName) {
    std::string path = "/session/minecraft/hasJoined?username=" + username + "&serverId=" + serverHash;

    HINTERNET hSession = WinHttpOpen(L"OurcraftServer/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"sessionserver.mojang.com",
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    std::wstring wpath(path.begin(), path.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath.c_str(), nullptr,
                                             WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {

        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

        if (statusCode == 200) {
            std::string body;
            DWORD available = 0;
            while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
                std::vector<char> buf(available);
                DWORD read = 0;
                WinHttpReadData(hRequest, buf.data(), available, &read);
                body.append(buf.data(), read);
            }

            auto extractField = [&](const std::string& field) -> std::string {
                std::string needle = "\"" + field + "\":\"";
                size_t pos = body.find(needle);
                if (pos == std::string::npos) return "";
                pos += needle.size();
                size_t end = body.find('"', pos);
                if (end == std::string::npos) return "";
                return body.substr(pos, end - pos);
            };

            std::string id = extractField("id");
            std::string name = extractField("name");

            if (!id.empty() && !name.empty()) {
                if (id.size() == 32) {
                    outUuid = id.substr(0, 8) + "-" + id.substr(8, 4) + "-" +
                              id.substr(12, 4) + "-" + id.substr(16, 4) + "-" + id.substr(20);
                } else {
                    outUuid = id;
                }
                outName = name;
                success = true;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

void JavaListener::sendPacket(JavaClient& client, int32_t packetId, const std::vector<uint8_t>& data) {
    uint8_t idBuf[5];
    int idBytes = VarInt::write(idBuf, packetId);

    int32_t totalLen = idBytes + (int32_t)data.size();
    uint8_t lenBuf[5];
    int lenBytes = VarInt::write(lenBuf, totalLen);

    std::vector<uint8_t> packet;
    packet.reserve(lenBytes + idBytes + data.size());
    packet.insert(packet.end(), lenBuf, lenBuf + lenBytes);
    packet.insert(packet.end(), idBuf, idBuf + idBytes);
    packet.insert(packet.end(), data.begin(), data.end());

    sendRaw(client, packet.data(), (int)packet.size());
}

void JavaListener::sendRaw(JavaClient& client, const uint8_t* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = ::send(client.sock, (const char*)(data + sent), len - sent, 0);
        if (n <= 0) {
            client.state = ClientState::Disconnected;
            return;
        }
        sent += n;
    }
}

std::string JavaListener::buildStatusJson() {
    std::ostringstream ss;
    std::string motd = toNarrow(m_motd);

    ss << "{";
    ss << "\"version\":{\"name\":\"" << VERSION_NAME << "\",\"protocol\":" << PROTOCOL_VERSION << "},";
    ss << "\"players\":{\"max\":" << m_maxPlayers << ",\"online\":" << m_onlinePlayers << "},";
    ss << "\"description\":{\"text\":\"" << jsonEscape(motd) << "\"},";
    ss << "\"enforcesSecureChat\":false,";
    ss << "\"previewsChat\":false";
    ss << "}";

    return ss.str();
}

std::string JavaListener::offlineUuid(const std::string& name) {
    // Java Edition offline UUID: MD5("OfflinePlayer:" + name), set version 3
    std::string input = "OfflinePlayer:" + name;
    uint8_t hash[16];
    simpleMd5((const uint8_t*)input.c_str(), input.size(), hash);

    char buf[37];
    snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        hash[0], hash[1], hash[2], hash[3],
        hash[4], hash[5], hash[6], hash[7],
        hash[8], hash[9], hash[10], hash[11],
        hash[12], hash[13], hash[14], hash[15]);
    return std::string(buf);
}

int JavaListener::getConnectionCount() const {
    int count = 0;
    for (const auto& c : m_clients) {
        if (c.state == ClientState::Play) count++;
    }
    return count;
}

static void writeInt(std::vector<uint8_t>& buf, int32_t val) {
    uint8_t b[4];
    b[0] = (val >> 24) & 0xFF;
    b[1] = (val >> 16) & 0xFF;
    b[2] = (val >> 8) & 0xFF;
    b[3] = val & 0xFF;
    buf.insert(buf.end(), b, b + 4);
}

static void writeLong(std::vector<uint8_t>& buf, int64_t val) {
    uint8_t b[8];
    for (int i = 7; i >= 0; i--) { b[7-i] = (val >> (i*8)) & 0xFF; }
    buf.insert(buf.end(), b, b + 8);
}

static void writeFloat(std::vector<uint8_t>& buf, float val) {
    uint32_t bits;
    memcpy(&bits, &val, 4);
    writeInt(buf, (int32_t)bits);
}

static void writeDouble(std::vector<uint8_t>& buf, double val) {
    uint64_t bits;
    memcpy(&bits, &val, 8);
    writeLong(buf, (int64_t)bits);
}

static void writeByte(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

static void writeBool(std::vector<uint8_t>& buf, bool val) {
    buf.push_back(val ? 1 : 0);
}

static void writeVarInt(std::vector<uint8_t>& buf, int32_t val) {
    uint8_t tmp[5];
    int n = VarInt::write(tmp, val);
    buf.insert(buf.end(), tmp, tmp + n);
}

static void writeString(std::vector<uint8_t>& buf, const std::string& s) {
    writeVarInt(buf, (int32_t)s.size());
    buf.insert(buf.end(), s.begin(), s.end());
}

static void writeShort(std::vector<uint8_t>& buf, int16_t val) {
    uint8_t b[2];
    b[0] = (val >> 8) & 0xFF;
    b[1] = val & 0xFF;
    buf.insert(buf.end(), b, b + 2);
}

static void writePosition(std::vector<uint8_t>& buf, int x, int y, int z) {
    // Java 1.8 packed position: x<<38 | y<<26 | z (26+12+26 bits)
    int64_t val = (((int64_t)x & 0x3FFFFFF) << 38) |
                  (((int64_t)y & 0xFFF) << 26) |
                  ((int64_t)z & 0x3FFFFFF);
    writeLong(buf, val);
}

void JavaListener::sendJoinGame(JavaClient& client) {
    std::vector<uint8_t> data;

    writeInt(data, (int32_t)client.connectionId);
    writeByte(data, 0);    // gamemode: 0=survival
    writeByte(data, 0);    // dimension: 0=overworld
    writeByte(data, 1);    // difficulty: 1=easy
    writeByte(data, (uint8_t)m_maxPlayers);
    writeString(data, "default");
    writeBool(data, false);

    sendPacket(client, 0x01, data);

    sendSpawnPosition(client);
    sendPlayerPositionAndLook(client);

    printf("[Java] Sent Join Game to %s\n", client.username.c_str());
}

void JavaListener::sendSpawnPosition(JavaClient& client) {
    std::vector<uint8_t> data;
    writePosition(data, 0, 64, 0);
    sendPacket(client, 0x05, data);
}

void JavaListener::sendPlayerPositionAndLook(JavaClient& client) {
    std::vector<uint8_t> data;
    writeDouble(data, 0.0);
    writeDouble(data, 66.0);
    writeDouble(data, 0.0);
    writeFloat(data, 0.0f);
    writeFloat(data, 0.0f);
    writeByte(data, 0); // flags=0: all absolute
    sendPacket(client, 0x08, data);
}

void JavaListener::sendKeepAlive(JavaClient& client) {
    client.keepAliveId++;
    std::vector<uint8_t> data;
    writeVarInt(data, client.keepAliveId);
    sendPacket(client, 0x00, data);
    client.lastKeepAlive = GetTickCount();
}

void JavaListener::handleKeepAliveResponse(JavaClient& client, const std::vector<uint8_t>& payload) {
    client.lastActivity = GetTickCount();
}

void JavaListener::sendDisconnect(JavaClient& client, const std::string& reason) {
    std::vector<uint8_t> data;
    std::string json = "{\"text\":\"" + jsonEscape(reason) + "\"}";
    writeString(data, json);
    sendPacket(client, 0x40, data);
    client.state = ClientState::Disconnected;
}

void JavaListener::sendChatMessage(JavaClient& client, const std::string& jsonMsg) {
    std::vector<uint8_t> data;
    writeString(data, jsonMsg);
    writeByte(data, 0); // 0=chat, 1=system, 2=action bar
    sendPacket(client, 0x02, data);
}

std::vector<std::string> JavaListener::getPlayerNames() const {
    std::vector<std::string> names;
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_clientsMtx));
    for (const auto& c : m_clients) {
        if (c.state == ClientState::Play && !c.username.empty())
            names.push_back(c.username);
    }
    return names;
}

bool JavaListener::kickPlayer(const std::string& name, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_clientsMtx);
    for (auto& c : m_clients) {
        if (c.state == ClientState::Play && c.username == name) {
            sendDisconnect(c, reason);
            return true;
        }
    }
    return false;
}

void JavaListener::broadcastChat(const std::string& message) {
    std::string json = "{\"text\":\"" + jsonEscape(message) + "\"}";
    std::lock_guard<std::mutex> lock(m_clientsMtx);
    for (auto& c : m_clients) {
        if (c.state == ClientState::Play)
            sendChatMessage(c, json);
    }
}

void JavaListener::disconnectAll(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_clientsMtx);
    for (auto& c : m_clients) {
        if (c.state == ClientState::Play)
            sendDisconnect(c, reason);
    }
}

void JavaListener::sendTransferPacket(JavaClient& client, uint8_t subType,
                                       const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload;
    payload.push_back(subType);
    payload.insert(payload.end(), data.begin(), data.end());
    sendPacket(client, ModTransfer::PACKET_MOD_TRANSFER, payload);
}

void JavaListener::sendModManifest(JavaClient& client) {
    auto& dist = ModDistributor::instance();
    auto encoded = ModTransfer::ModTransferCodec::encode(dist.getModManifest());
    sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::ModManifest), encoded);
    printf("[Java] Sent mod manifest to %s\n", client.username.c_str());
}

void JavaListener::sendResourceManifest(JavaClient& client) {
    auto& dist = ModDistributor::instance();
    auto encoded = ModTransfer::ModTransferCodec::encode(dist.getResourceManifest());
    sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::ResourceManifest), encoded);
    printf("[Java] Sent resource manifest to %s\n", client.username.c_str());
}

void JavaListener::sendFileChunks(JavaClient& client, const uint8_t clientX25519Key[32]) {
    auto& dist = ModDistributor::instance();
    const int CHUNK_SIZE = 65536;

    for (const auto& mod : dist.getMods()) {
        auto encrypted = dist.encryptForClient(mod, clientX25519Key);
        uint32_t offset = 0;
        while (offset < (uint32_t)encrypted.size()) {
            uint32_t chunkLen = std::min((uint32_t)CHUNK_SIZE, (uint32_t)(encrypted.size() - offset));
            ModTransfer::FileChunk chunk = {};
            memcpy(chunk.fileHash, mod.fileHash, 32);
            chunk.offset = offset;
            chunk.totalSize = (uint32_t)encrypted.size();
            chunk.data.assign(encrypted.data() + offset, encrypted.data() + offset + chunkLen);
            auto encoded = ModTransfer::ModTransferCodec::encode(chunk);
            sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::FileChunk), encoded);
            offset += chunkLen;
        }
        std::vector<uint8_t> completeData(mod.fileHash, mod.fileHash + 32);
        sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::TransferComplete), completeData);
    }

    for (const auto& pack : dist.getPacks()) {
        const auto& fileData = pack.fileData;
        uint32_t offset = 0;
        while (offset < (uint32_t)fileData.size()) {
            uint32_t chunkLen = std::min((uint32_t)CHUNK_SIZE, (uint32_t)(fileData.size() - offset));
            ModTransfer::FileChunk chunk = {};
            memcpy(chunk.fileHash, pack.fileHash, 32);
            chunk.offset = offset;
            chunk.totalSize = (uint32_t)fileData.size();
            chunk.data.assign(fileData.data() + offset, fileData.data() + offset + chunkLen);
            auto encoded = ModTransfer::ModTransferCodec::encode(chunk);
            sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::FileChunk), encoded);
            offset += chunkLen;
        }
        std::vector<uint8_t> completeData(pack.fileHash, pack.fileHash + 32);
        sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::TransferComplete), completeData);
    }
}

void JavaListener::handleModTransferResponse(JavaClient& client, const std::vector<uint8_t>& payload) {
    if (payload.empty()) return;
    if (payload[0] != static_cast<uint8_t>(ModTransfer::TransferSubType::ClientResponse)) return;

    ModTransfer::ClientResponse resp = {};
    if (!ModTransfer::ModTransferCodec::decode(payload.data() + 1, payload.size() - 1, resp)) return;

    switch (resp.type) {
        case ModTransfer::ClientResponseType::AcceptMods:
            printf("[Java] %s accepted mod download\n", client.username.c_str());
            memcpy(client.clientX25519Key, resp.x25519PublicKey, 32);
            sendFileChunks(client, client.clientX25519Key);
            break;

        case ModTransfer::ClientResponseType::DeclineMods:
            printf("[Java] %s declined mods — sending resource packs only\n", client.username.c_str());
            {
                auto& dist = ModDistributor::instance();
                const int CHUNK_SIZE = 65536;
                for (const auto& pack : dist.getPacks()) {
                    const auto& fileData = pack.fileData;
                    uint32_t offset = 0;
                    while (offset < (uint32_t)fileData.size()) {
                        uint32_t chunkLen = std::min((uint32_t)CHUNK_SIZE, (uint32_t)(fileData.size() - offset));
                        ModTransfer::FileChunk chunk = {};
                        memcpy(chunk.fileHash, pack.fileHash, 32);
                        chunk.offset = offset;
                        chunk.totalSize = (uint32_t)fileData.size();
                        chunk.data.assign(fileData.data() + offset, fileData.data() + offset + chunkLen);
                        auto encoded = ModTransfer::ModTransferCodec::encode(chunk);
                        sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::FileChunk), encoded);
                        offset += chunkLen;
                    }
                    std::vector<uint8_t> completeData(pack.fileHash, pack.fileHash + 32);
                    sendTransferPacket(client, static_cast<uint8_t>(ModTransfer::TransferSubType::TransferComplete), completeData);
                }
            }
            break;

        case ModTransfer::ClientResponseType::Ready:
            printf("[Java] %s transfer complete, spawning\n", client.username.c_str());
            client.inTransferPhase = false;
            client.transferComplete = true;
            sendJoinGame(client);
            break;
    }
}
