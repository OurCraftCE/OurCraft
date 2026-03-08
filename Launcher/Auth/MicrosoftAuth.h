#pragma once
#include <string>
#include <functional>
#include <cstdint>

struct AuthTokens {
    std::string microsoftAccessToken;
    std::string microsoftRefreshToken;
    std::string xboxLiveToken;
    std::string xboxUserHash;
    std::string xstsToken;
    std::string minecraftToken;
    std::string username;
    std::string uuid;
    int64_t expiresAt = 0;
};

class MicrosoftAuth {
public:
    static constexpr const char* DEFAULT_CLIENT_ID = "00000000-0000-0000-0000-000000000000"; // placeholder
    static constexpr int REDIRECT_PORT = 27015;

    void SetClientId(const std::string& clientId) { m_ClientId = clientId; }
    const std::string& GetClientId() const { return m_ClientId; }

    bool Authenticate(AuthTokens& tokens, std::function<void(const std::string&)> progress = nullptr);
    bool Refresh(AuthTokens& tokens, std::function<void(const std::string&)> progress = nullptr);
    bool IsValid(const AuthTokens& tokens) const;

private:
    std::string WaitForAuthorizationCode();
    bool ExchangeCodeForTokens(const std::string& code, AuthTokens& tokens);
    bool AuthenticateXboxLive(AuthTokens& tokens);
    bool AuthenticateXSTS(AuthTokens& tokens);
    bool AuthenticateMinecraft(AuthTokens& tokens);
    bool CheckBedrockOwnership(AuthTokens& tokens);

    std::string m_ClientId = DEFAULT_CLIENT_ID;
};
