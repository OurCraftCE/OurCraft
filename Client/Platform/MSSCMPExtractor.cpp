#include "MSSCMPExtractor.h"
#include "BinkAudioDecoder.h"

#include <cstdio>
#include <cstring>
#include <direct.h>
#include <sys/stat.h>

static void EnsureDirectoryExists(const char *path) {
    std::string dir(path);
    for (size_t i = 0; i < dir.size(); i++) {
        if (dir[i] == '/' || dir[i] == '\\') {
            dir[i] = '\0';
            _mkdir(dir.c_str());
            dir[i] = '\\';
        }
    }
    _mkdir(dir.c_str());
}

static bool FileExists(const char *path) {
    struct _stat st;
    return _stat(path, &st) == 0;
}

// read a little-endian uint32 from buffer
static uint32_t ReadU32(const uint8_t *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

bool MSSCMPExtractor::ParseMSSCMP(const char *bankPath, std::vector<SoundEntry> &entries) {
    FILE *f = fopen(bankPath, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t header[0x60];
    if (fread(header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return false;
    }

    if (memcmp(header, "KNAB", 4) != 0) {
        fclose(f);
        return false;
    }

    uint32_t eventCount = ReadU32(header + 0x40);
    uint32_t eventTableOff = 0x5C;
    uint32_t soundTableOff = eventTableOff + eventCount * 8;

    fseek(f, soundTableOff, SEEK_SET);
    std::vector<std::pair<uint32_t, uint32_t>> soundTable;

    while (ftell(f) < fileSize - 8) {
        uint8_t buf[8];
        if (fread(buf, 1, 8, f) != 8) break;
        uint32_t nameOff = ReadU32(buf);
        uint32_t metaOff = ReadU32(buf + 4);

        if (nameOff < 0xE000 || nameOff > (uint32_t)fileSize)
            break;

        soundTable.push_back({nameOff, metaOff});
    }

    entries.reserve(soundTable.size());
    for (auto &[nameOff, metaOff] : soundTable) {
        char nameBuf[256] = {};
        fseek(f, nameOff, SEEK_SET);
        fread(nameBuf, 1, 255, f);
        nameBuf[255] = 0;

        uint8_t meta[60];
        fseek(f, metaOff, SEEK_SET);
        if (fread(meta, 1, 60, f) != 60) continue;

        uint32_t audioOffset = ReadU32(meta + 8);
        uint32_t channels = ReadU32(meta + 12);
        uint32_t sampleRate = ReadU32(meta + 20);
        uint32_t dataSize = ReadU32(meta + 24);

        if (audioOffset == 0 || dataSize == 0 || audioOffset + dataSize > (uint32_t)fileSize)
            continue;

        SoundEntry e;
        e.name = nameBuf;
        e.audioOffset = audioOffset;
        e.audioSize = dataSize;
        e.channels = (int)channels;
        e.sampleRate = (int)sampleRate;
        entries.push_back(e);
    }

    fclose(f);
    return !entries.empty();
}

std::string MSSCMPExtractor::SoundNameToFilePath(const std::string &name, const char *outputDir) {
    std::string sname = name;
    const char *prefix = "Minecraft/";
    size_t prefixLen = strlen(prefix);
    if (sname.compare(0, prefixLen, prefix) == 0)
        sname = sname.substr(prefixLen);

    for (auto &c : sname) {
        if (c == '/') c = '_';
    }

    return std::string(outputDir) + "\\" + sname + ".wav";
}

bool MSSCMPExtractor::WriteWAV(const char *path, const void *pcmData, uint32_t pcmSize,
                                int channels, int sampleRate, int bitsPerSample) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint16_t blockAlign = (uint16_t)(channels * bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    uint32_t totalSize = 36 + pcmSize;

    fwrite("RIFF", 1, 4, f);
    fwrite(&totalSize, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    uint32_t fmtSize = 16;
    fwrite(&fmtSize, 4, 1, f);
    uint16_t audioFormat = 1;
    fwrite(&audioFormat, 2, 1, f);
    uint16_t ch = (uint16_t)channels;
    fwrite(&ch, 2, 1, f);
    uint32_t sr = (uint32_t)sampleRate;
    fwrite(&sr, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&blockAlign, 2, 1, f);
    uint16_t bps = (uint16_t)bitsPerSample;
    fwrite(&bps, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&pcmSize, 4, 1, f);
    fwrite(pcmData, 1, pcmSize, f);

    fclose(f);
    return true;
}

static bool ExtractWithBinkDecoder(const std::vector<MSSCMPExtractor::SoundEntry> &entries,
                                    const uint8_t *fileData, size_t fileSize,
                                    const char *outputDir) {
    EnsureDirectoryExists(outputDir);

    int extracted = 0;
    int failed = 0;

    for (const auto &entry : entries) {
        std::string outPath = MSSCMPExtractor::SoundNameToFilePath(entry.name, outputDir);

        if (FileExists(outPath.c_str())) {
            extracted++;
            continue;
        }

        const uint8_t *audioData = fileData + entry.audioOffset;
        uint32_t audioSize = entry.audioSize;

        BinkAudioDecoder::Result result;
        if (BinkAudioDecoder::Decode1FCB(audioData, audioSize, result)) {
            if (BinkAudioDecoder::WriteWAV(outPath.c_str(), result)) {
                extracted++;
            } else {
                failed++;
            }
        } else {
            printf("MSSCMPExtractor: Failed to decode '%s'\n", entry.name.c_str());
            failed++;
        }
    }

    printf("MSSCMPExtractor: Extracted %d/%d sounds (%d failed)\n",
           extracted, (int)entries.size(), failed);

    return extracted > 0;
}

// C-linkage wrapper for calling from PCH-based files
extern "C" int MSSCMP_ExtractSoundBank(const char *bankPath, const char *outputDir) {
    return MSSCMPExtractor::ExtractSoundBank(bankPath, outputDir);
}

int MSSCMPExtractor::ExtractSoundBank(const char *bankPath, const char *outputDir) {
    std::string markerPath = std::string(outputDir) + "\\.extracted";
    if (FileExists(markerPath.c_str())) {
        printf("MSSCMPExtractor: Sounds already extracted, skipping\n");
        return 0;
    }

    if (!FileExists(bankPath)) {
        printf("MSSCMPExtractor: Soundbank '%s' not found\n", bankPath);
        return -1;
    }

    std::vector<SoundEntry> entries;
    if (!ParseMSSCMP(bankPath, entries)) {
        printf("MSSCMPExtractor: Failed to parse '%s'\n", bankPath);
        return -1;
    }
    printf("MSSCMPExtractor: Found %d sounds in soundbank\n", (int)entries.size());

    FILE *f = fopen(bankPath, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *fileData = new uint8_t[fileSize];
    fread(fileData, 1, fileSize, f);
    fclose(f);

    bool ok = ExtractWithBinkDecoder(entries, fileData, fileSize, outputDir);
    delete[] fileData;

    if (ok) {
        FILE *marker = fopen(markerPath.c_str(), "w");
        if (marker) {
            fprintf(marker, "Extracted %d sounds from %s\n", (int)entries.size(), bankPath);
            fclose(marker);
        }
    }

    return ok ? (int)entries.size() : -1;
}
