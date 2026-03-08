#pragma once
#include <cstdint>
#include <cstring>

// Java Edition VarInt encoding (LEB128 variant)
namespace VarInt {

// returns bytes written
inline int write(uint8_t* buf, int32_t value) {
    uint32_t uval = static_cast<uint32_t>(value);
    int i = 0;
    while (uval & ~0x7F) {
        buf[i++] = static_cast<uint8_t>((uval & 0x7F) | 0x80);
        uval >>= 7;
    }
    buf[i++] = static_cast<uint8_t>(uval);
    return i;
}

// returns bytes consumed, or 0 on error; result in *out
inline int read(const uint8_t* buf, int bufLen, int32_t* out) {
    uint32_t result = 0;
    int shift = 0;
    int i = 0;
    while (i < bufLen && i < 5) {
        uint8_t b = buf[i++];
        result |= (static_cast<uint32_t>(b & 0x7F)) << shift;
        if ((b & 0x80) == 0) {
            *out = static_cast<int32_t>(result);
            return i;
        }
        shift += 7;
    }
    return 0; // incomplete or overflow
}

// encoded byte length of value
inline int size(int32_t value) {
    uint32_t uval = static_cast<uint32_t>(value);
    int bytes = 0;
    do {
        uval >>= 7;
        bytes++;
    } while (uval);
    return bytes;
}

} // namespace VarInt
