#pragma once
#ifdef __EMSCRIPTEN__

#include <cstdint>
#include <functional>
#include <string>

// ---------------------------------------------------------------------------
// EmscriptenWebSocket — thin networking shim for WASM builds.
// Replaces RakNet raw UDP by routing packets over a WebSocket relay server.
// The relay server (Node.js/Go) bridges WASM clients with Windows RakNet peers.
//
// Usage:
//   EmscriptenWebSocket::Connect("ws://server:19132/relay",
//       [](const uint8_t* data, size_t len) { /* on packet */ },
//       []() { /* on open */ },
//       []() { /* on close */ });
//   EmscriptenWebSocket::Send(data, len);
// ---------------------------------------------------------------------------

class EmscriptenWebSocket
{
public:
    using OnPacket = std::function<void(const uint8_t*, size_t)>;
    using OnEvent  = std::function<void()>;

    static bool Connect(const std::string& url,
                        OnPacket onPacket,
                        OnEvent  onOpen  = {},
                        OnEvent  onClose = {});

    static void Send(const uint8_t* data, size_t len);
    static void Close();
    static bool IsConnected();

private:
    EmscriptenWebSocket() = delete;
};

#endif // __EMSCRIPTEN__
