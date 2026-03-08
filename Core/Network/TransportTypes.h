#pragma once
#include <cstdint>
#include <functional>

struct ConnectionId {
    uint64_t id = 0;
    bool operator==(const ConnectionId& o) const { return id == o.id; }
    bool operator!=(const ConnectionId& o) const { return id != o.id; }
    bool isValid() const { return id != 0; }
};

struct ConnectionId_Hash {
    size_t operator()(const ConnectionId& c) const { return std::hash<uint64_t>()(c.id); }
};

enum class PacketPriority : uint8_t {
    Immediate = 0, High, Medium, Low
};

enum class PacketReliability : uint8_t {
    Unreliable = 0, UnreliableSequenced, Reliable, ReliableOrdered, ReliableSequenced
};

struct IncomingPacket {
    ConnectionId sender;
    uint8_t* data = nullptr;
    uint32_t length = 0;
};
