#ifdef __EMSCRIPTEN__

#include "EmscriptenWebSocket.h"
#include <emscripten/websocket.h>
#include <emscripten/emscripten.h>
#include <cstring>

static EMSCRIPTEN_WEBSOCKET_T  s_socket     = 0;
static bool                    s_connected  = false;
static EmscriptenWebSocket::OnPacket s_onPacket;
static EmscriptenWebSocket::OnEvent  s_onOpen;
static EmscriptenWebSocket::OnEvent  s_onClose;

static EM_BOOL OnOpen(int, const EmscriptenWebSocketOpenEvent*, void*)
{
    s_connected = true;
    if (s_onOpen) s_onOpen();
    return EM_TRUE;
}

static EM_BOOL OnMessage(int, const EmscriptenWebSocketMessageEvent* e, void*)
{
    if (e->isBinary && s_onPacket)
        s_onPacket(reinterpret_cast<const uint8_t*>(e->data), e->numBytes);
    return EM_TRUE;
}

static EM_BOOL OnClose(int, const EmscriptenWebSocketCloseEvent*, void*)
{
    s_connected = false;
    if (s_onClose) s_onClose();
    return EM_TRUE;
}

static EM_BOOL OnError(int, const EmscriptenWebSocketErrorEvent*, void*)
{
    s_connected = false;
    return EM_TRUE;
}

bool EmscriptenWebSocket::Connect(const std::string& url,
                                  OnPacket onPacket,
                                  OnEvent  onOpen,
                                  OnEvent  onClose)
{
    if (s_socket)
        emscripten_websocket_close(s_socket, 1000, "reconnect");

    s_onPacket = std::move(onPacket);
    s_onOpen   = std::move(onOpen);
    s_onClose  = std::move(onClose);

    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);
    attr.url = url.c_str();

    s_socket = emscripten_websocket_new(&attr);
    if (s_socket <= 0) return false;

    emscripten_websocket_set_onopen_callback (s_socket, nullptr, OnOpen);
    emscripten_websocket_set_onmessage_callback(s_socket, nullptr, OnMessage);
    emscripten_websocket_set_onclose_callback(s_socket, nullptr, OnClose);
    emscripten_websocket_set_onerror_callback(s_socket, nullptr, OnError);

    return true;
}

void EmscriptenWebSocket::Send(const uint8_t* data, size_t len)
{
    if (s_connected && s_socket > 0)
        emscripten_websocket_send_binary(s_socket,
            const_cast<void*>(static_cast<const void*>(data)),
            static_cast<uint32_t>(len));
}

void EmscriptenWebSocket::Close()
{
    if (s_socket > 0)
    {
        emscripten_websocket_close(s_socket, 1000, "client disconnect");
        s_socket    = 0;
        s_connected = false;
    }
}

bool EmscriptenWebSocket::IsConnected()
{
    return s_connected;
}

#endif // __EMSCRIPTEN__
