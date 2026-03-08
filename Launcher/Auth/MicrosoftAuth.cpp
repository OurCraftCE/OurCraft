#include "MicrosoftAuth.h"
#include "HttpClient.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <shellapi.h>

#include <nlohmann/json.hpp>

#include <chrono>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

// ---- helpers ----

static std::string UrlEncode(const std::string& value) {
    std::ostringstream escaped;
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase;
            escaped << std::hex << ((c >> 4) & 0xF) << (c & 0xF);
            escaped << std::nouppercase;
        }
    }
    return escaped.str();
}

static int64_t NowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ---- MicrosoftAuth ----

bool MicrosoftAuth::Authenticate(AuthTokens& tokens, std::function<void(const std::string&)> progress) {
    if (progress) progress("Opening browser for Microsoft login...");
    std::string code = WaitForAuthorizationCode();
    if (code.empty()) return false;

    if (progress) progress("Exchanging authorization code...");
    if (!ExchangeCodeForTokens(code, tokens)) return false;

    if (progress) progress("Authenticating with Xbox Live...");
    if (!AuthenticateXboxLive(tokens)) return false;

    if (progress) progress("Getting XSTS token...");
    if (!AuthenticateXSTS(tokens)) return false;

    if (progress) progress("Authenticating with Minecraft...");
    if (!AuthenticateMinecraft(tokens)) return false;

    if (progress) progress("Checking game ownership...");
    if (!CheckBedrockOwnership(tokens)) return false;

    if (progress) progress("Authentication complete!");
    return true;
}

bool MicrosoftAuth::Refresh(AuthTokens& tokens, std::function<void(const std::string&)> progress) {
    if (tokens.microsoftRefreshToken.empty()) return false;

    if (progress) progress("Refreshing Microsoft token...");

    HttpClient http;
    std::string body = "client_id=" + m_ClientId
        + "&refresh_token=" + UrlEncode(tokens.microsoftRefreshToken)
        + "&grant_type=refresh_token"
        + "&redirect_uri=" + UrlEncode("http://localhost:" + std::to_string(REDIRECT_PORT))
        + "&scope=" + UrlEncode("XboxLive.signin offline_access");

    auto resp = http.Post("https://login.microsoftonline.com/consumers/oauth2/v2.0/token", body);
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.microsoftAccessToken = j.value("access_token", "");
        tokens.microsoftRefreshToken = j.value("refresh_token", tokens.microsoftRefreshToken);
        int expiresIn = j.value("expires_in", 3600);
        tokens.expiresAt = NowSeconds() + expiresIn;
    } catch (...) {
        return false;
    }

    if (progress) progress("Authenticating with Xbox Live...");
    if (!AuthenticateXboxLive(tokens)) return false;

    if (progress) progress("Getting XSTS token...");
    if (!AuthenticateXSTS(tokens)) return false;

    if (progress) progress("Authenticating with Minecraft...");
    if (!AuthenticateMinecraft(tokens)) return false;

    if (progress) progress("Checking game ownership...");
    if (!CheckBedrockOwnership(tokens)) return false;

    if (progress) progress("Refresh complete!");
    return true;
}

bool MicrosoftAuth::IsValid(const AuthTokens& tokens) const {
    return !tokens.minecraftToken.empty() && tokens.expiresAt > NowSeconds();
}

std::string MicrosoftAuth::WaitForAuthorizationCode() {
	// Thanks to jonny for this part
    // Build auth URL
    std::string authUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"
        "?client_id=" + m_ClientId +
        "&response_type=code"
        "&redirect_uri=" + UrlEncode("http://localhost:" + std::to_string(REDIRECT_PORT)) +
        "&scope=" + UrlEncode("XboxLive.signin offline_access") +
        "&prompt=select_account";

    // Open browser
    ShellExecuteA(nullptr, "open", authUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    // Start Winsock TCP server
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return "";

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) { WSACleanup(); return ""; }

    // Allow address reuse
    int optVal = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optVal), sizeof(optVal));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<u_short>(REDIRECT_PORT));

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return "";
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return "";
    }

    // Set 120 second timeout for accept
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(listenSock, &readSet);
    timeval tv;
    tv.tv_sec = 120;
    tv.tv_usec = 0;

    int selResult = select(0, &readSet, nullptr, nullptr, &tv);
    if (selResult <= 0) {
        closesocket(listenSock);
        WSACleanup();
        return "";
    }

    SOCKET clientSock = accept(listenSock, nullptr, nullptr);
    closesocket(listenSock);

    if (clientSock == INVALID_SOCKET) {
        WSACleanup();
        return "";
    }

    // Read HTTP request
    char buf[4096];
    int bytesRead = recv(clientSock, buf, sizeof(buf) - 1, 0);
    std::string code;

    if (bytesRead > 0) {
        buf[bytesRead] = '\0';
        std::string request(buf);

        // Parse code from GET /?code=XXXXX HTTP/1.1
        auto codePos = request.find("code=");
        if (codePos != std::string::npos) {
            auto codeStart = codePos + 5;
            auto codeEnd = request.find_first_of("& \r\n", codeStart);
            if (codeEnd == std::string::npos) codeEnd = request.size();
            code = request.substr(codeStart, codeEnd - codeStart);
        }
    }

    // Send success response
    const char* successHtml =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "<html><body><h1>Authentication successful!</h1>"
        "<p>You can close this window and return to the launcher.</p></body></html>";
    send(clientSock, successHtml, static_cast<int>(strlen(successHtml)), 0);

    closesocket(clientSock);
    WSACleanup();
    return code;
}

bool MicrosoftAuth::ExchangeCodeForTokens(const std::string& code, AuthTokens& tokens) {
    HttpClient http;
    std::string body = "client_id=" + m_ClientId
        + "&code=" + UrlEncode(code)
        + "&grant_type=authorization_code"
        + "&redirect_uri=" + UrlEncode("http://localhost:" + std::to_string(REDIRECT_PORT))
        + "&scope=" + UrlEncode("XboxLive.signin offline_access");

    auto resp = http.Post("https://login.microsoftonline.com/consumers/oauth2/v2.0/token", body);
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.microsoftAccessToken = j.value("access_token", "");
        tokens.microsoftRefreshToken = j.value("refresh_token", "");
        int expiresIn = j.value("expires_in", 3600);
        tokens.expiresAt = NowSeconds() + expiresIn;
        return !tokens.microsoftAccessToken.empty();
    } catch (...) {
        return false;
    }
}

bool MicrosoftAuth::AuthenticateXboxLive(AuthTokens& tokens) {
    HttpClient http;
    json body = {
        {"Properties", {
            {"AuthMethod", "RPS"},
            {"SiteName", "user.auth.xboxlive.com"},
            {"RpsTicket", "d=" + tokens.microsoftAccessToken}
        }},
        {"RelyingParty", "http://auth.xboxlive.com"},
        {"TokenType", "JWT"}
    };

    auto resp = http.Post("https://user.auth.xboxlive.com/user/authenticate",
                          body.dump(), "application/json",
                          {{"Accept", "application/json"}});
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.xboxLiveToken = j.value("Token", "");
        tokens.xboxUserHash = j["DisplayClaims"]["xui"][0].value("uhs", "");
        return !tokens.xboxLiveToken.empty();
    } catch (...) {
        return false;
    }
}

bool MicrosoftAuth::AuthenticateXSTS(AuthTokens& tokens) {
    HttpClient http;
    json body = {
        {"Properties", {
            {"SandboxId", "RETAIL"},
            {"UserTokens", json::array({tokens.xboxLiveToken})}
        }},
        {"RelyingParty", "rp://api.minecraftservices.com/"},
        {"TokenType", "JWT"}
    };

    auto resp = http.Post("https://xsts.auth.xboxlive.com/xsts/authorize",
                          body.dump(), "application/json",
                          {{"Accept", "application/json"}});
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.xstsToken = j.value("Token", "");
        return !tokens.xstsToken.empty();
    } catch (...) {
        return false;
    }
}

bool MicrosoftAuth::AuthenticateMinecraft(AuthTokens& tokens) {
    HttpClient http;
    json body = {
        {"identityToken", "XBL3.0 x=" + tokens.xboxUserHash + ";" + tokens.xstsToken}
    };

    auto resp = http.Post("https://api.minecraftservices.com/authentication/login_with_xbox",
                          body.dump(), "application/json");
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.minecraftToken = j.value("access_token", "");
        int expiresIn = j.value("expires_in", 86400);
        tokens.expiresAt = NowSeconds() + expiresIn;
        return !tokens.minecraftToken.empty();
    } catch (...) {
        return false;
    }
}

bool MicrosoftAuth::CheckBedrockOwnership(AuthTokens& tokens) {
    HttpClient http;
    std::map<std::string, std::string> authHeader = {
        {"Authorization", "Bearer " + tokens.minecraftToken}
    };

    // Check entitlements
    auto resp = http.Get("https://api.minecraftservices.com/entitlements/mcstore", authHeader);
    if (resp.statusCode != 200) return false;

    // Get profile (username + uuid)
    resp = http.Get("https://api.minecraftservices.com/minecraft/profile", authHeader);
    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        tokens.username = j.value("name", "");
        tokens.uuid = j.value("id", "");
        return !tokens.username.empty();
    } catch (...) {
        return false;
    }
}
