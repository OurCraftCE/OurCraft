#pragma once
#include <string>
#include <vector>

// extracts sounds from a Miles Sound System .msscmp soundbank
// using standalone Bink Audio decoder (no mss64.dll needed)
// writes individual WAV files that SDLAudioEngine can load

class MSSCMPExtractor {
public:
    struct SoundEntry {
        std::string name;      // e.g. "Minecraft/ambient/cave/cave10"
        uint32_t audioOffset;  // absolute file offset to 1FCB data
        uint32_t audioSize;    // size of 1FCB data in bytes
        int channels;
        int sampleRate;
    };

    // returns count extracted, -1 on failure; skips if already extracted
    static int ExtractSoundBank(const char *bankPath, const char *outputDir);

    static std::string SoundNameToFilePath(const std::string &name, const char *outputDir);

private:
    static bool ParseMSSCMP(const char *bankPath, std::vector<SoundEntry> &entries);
    static bool WriteWAV(const char *path, const void *pcmData, uint32_t pcmSize,
                         int channels, int sampleRate, int bitsPerSample);
};
