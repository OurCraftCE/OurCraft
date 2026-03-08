#pragma once
// Standalone Bink Audio (1FCB) decoder
// Based on the Bink Audio format spec (wiki.multimedia.cx/index.php/Bink_Audio)
// and FFmpeg's binkaudio.c (LGPL 2.1, Peter Ross, Daniel Verkamp)
//
// Decodes 1FCB blobs (Miles Sound System Bink Audio) to 16-bit PCM WAV.
// Zero external dependencies — self-contained RDFT implementation

#include <cstdint>
#include <vector>

class BinkAudioDecoder {
public:
    struct Result {
        std::vector<int16_t> pcmData;
        int channels;
        int sampleRate;
    };

    // decode 1FCB blob from MSSCMP soundbank to 16-bit PCM
    static bool Decode1FCB(const uint8_t *data, uint32_t dataSize, Result &result);

    static bool WriteWAV(const char *path, const Result &result);
};
