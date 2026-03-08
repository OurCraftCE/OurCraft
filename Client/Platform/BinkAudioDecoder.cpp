// Standalone Bink Audio (1FCB) decoder
// Based on the Bink Audio format spec and FFmpeg's binkaudio.c
// Original FFmpeg code: Copyright (c) 2007-2011 Peter Ross, 2009 Daniel Verkamp
// LGPL 2.1+ — this reimplementation uses the same algorithm.

#include "BinkAudioDecoder.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// bitstream reader (little-endian, matching FFmpeg's BITSTREAM_READER_LE)
struct BitReader {
    const uint8_t *data;
    int totalBits;
    int pos; // current bit position

    void init(const uint8_t *d, int sizeBytes) {
        data = d;
        totalBits = sizeBytes * 8;
        pos = 0;
    }

    int bitsLeft() const { return totalBits - pos; }

    uint32_t read(int n) {
        if (n == 0) return 0;
        uint32_t result = 0;
        for (int i = 0; i < n; i++) {
            if (pos >= totalBits) return result;
            int byteIdx = pos >> 3;
            int bitIdx = pos & 7;
            result |= ((uint32_t)((data[byteIdx] >> bitIdx) & 1)) << i;
            pos++;
        }
        return result;
    }

    uint32_t read1() { return read(1); }

    void skip(int n) { pos += n; }

    void alignTo32() {
        int n = (-pos) & 31;
        if (n) pos += n;
    }
};

// N-point inverse FFT (Cooley-Tukey, interleaved complex, power-of-2)
// computes x[n] = sum X[k]*exp(+2πi*k*n/N) (unnormalized)
static void ifft_interleaved(float *z, int n) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(z[2 * i], z[2 * j]);
            std::swap(z[2 * i + 1], z[2 * j + 1]);
        }
    }

    // positive angle = inverse transform
    for (int len = 2; len <= n; len <<= 1) {
        float angle = 2.0f * (float)M_PI / (float)len;
        float wRe = cosf(angle);
        float wIm = sinf(angle);
        for (int i = 0; i < n; i += len) {
            float curRe = 1.0f, curIm = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                int a = i + j;
                int b = i + j + len / 2;
                float tRe = z[2 * b] * curRe - z[2 * b + 1] * curIm;
                float tIm = z[2 * b] * curIm + z[2 * b + 1] * curRe;
                z[2 * b] = z[2 * a] - tRe;
                z[2 * b + 1] = z[2 * a + 1] - tIm;
                z[2 * a] += tRe;
                z[2 * a + 1] += tIm;
                float newRe = curRe * wRe - curIm * wIm;
                float newIm = curRe * wIm + curIm * wRe;
                curRe = newRe;
                curIm = newIm;
            }
        }
    }
}

// inverse RDFT: packed input data[0]=DC, data[1]=Nyquist, data[2k/2k+1]=Re/Im bins
// output: N real time-domain samples in data[0..N-1]
static void rdft_inverse(float *data, int n) {
    const int n2 = n / 2;

    std::vector<float> z(2 * n, 0.0f);
    z[0] = data[0]; z[1] = 0; // DC
    z[2 * n2] = data[1]; z[2 * n2 + 1] = 0; // Nyquist
    for (int k = 1; k < n2; k++) {
        z[2 * k] = data[2 * k];
        z[2 * k + 1] = data[2 * k + 1];
        // X[N-k] = conj(X[k])
        z[2 * (n - k)] = data[2 * k];
        z[2 * (n - k) + 1] = -data[2 * k + 1];
    }

    ifft_interleaved(z.data(), n);

    // imaginary parts are ~0 due to conjugate symmetry
    for (int i = 0; i < n; i++)
        data[i] = z[2 * i];
}

// DCT-III (inverse of DCT-II), coeffs[0] scaled by 1/sqrt(2)
static void dct_inverse(const float *coeffs, float *out, int n) {
    for (int k = 0; k < n; k++) {
        float sum = coeffs[0] * 0.5f;
        for (int i = 1; i < n; i++) {
            sum += coeffs[i] * cosf((float)M_PI * (2 * k + 1) * i / (2.0f * n));
        }
        out[k] = sum * (2.0f / n);
    }
}

// WMA critical frequency bands (from FFmpeg wma_freqs.h)
static const uint16_t wma_critical_freqs[25] = {
    100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720,
    2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000,
    15500, 24500
};

// RLE length table (from FFmpeg binkaudio.c)
static const uint8_t rle_length_tab[16] = {
    2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64
};

struct BinkAudioState {
    int channels;
    int sampleRate;
    int frameLen;
    int overlapLen;
    int blockSize;
    int numBands;
    float root;
    unsigned int bands[26];
    float quantTable[96];
    bool useDCT;
    bool versionB;
    bool first;

    // Previous frame overlap buffer (per channel)
    float previous[2][512]; // overlapLen max = frameLen/16 = 8192/16 = 512

    void init(int ch, int sr, bool dct, bool verB, int frameLenBitsOverride = 0) {
        channels = ch;
        sampleRate = sr;
        useDCT = dct;
        versionB = verB;
        first = true;

        int frameLenBits;
        if (frameLenBitsOverride > 0) {
            frameLenBits = frameLenBitsOverride;
        } else {
            if (sr < 22050) frameLenBits = 9;
            else if (sr < 44100) frameLenBits = 10;
            else frameLenBits = 11;
        }

        int effectiveSR = sr;
        int effectiveCh = ch;

        if (!useDCT) {
            // RDFT mode: multiply sample rate by channels, treat as mono
            effectiveSR = sr * ch;
            effectiveCh = 1;
            if (!versionB) {
                // frame_len_bits += log2(channels)
                int c = ch;
                while (c > 1) { frameLenBits++; c >>= 1; }
            }
        }

        frameLen = 1 << frameLenBits;
        overlapLen = frameLen / 16;
        blockSize = (frameLen - overlapLen) * (effectiveCh < 2 ? effectiveCh : 2);

        int srHalf = (effectiveSR + 1) / 2;

        if (!useDCT)
            root = 2.0f / (sqrtf((float)frameLen) * 32768.0f);
        else
            root = (float)frameLen / (sqrtf((float)frameLen) * 32768.0f);

        for (int i = 0; i < 96; i++)
            quantTable[i] = expf(i * 0.15289164787221953823f) * root;

        numBands = 1;
        for (; numBands < 25; numBands++)
            if (srHalf <= (int)wma_critical_freqs[numBands - 1])
                break;

        bands[0] = 2;
        for (int i = 1; i < numBands; i++)
            bands[i] = (wma_critical_freqs[i - 1] * frameLen / srHalf) & ~1;
        bands[numBands] = frameLen;

        memset(previous, 0, sizeof(previous));
    }

    bool decodeBlock(BitReader &gb, float **out) {
        int chCount = useDCT ? channels : 1;

        if (useDCT) {
            if (gb.bitsLeft() < 2) return false;
            gb.skip(2);
        }

        for (int ch = 0; ch < chCount; ch++) {
            std::vector<float> coeffs(frameLen + 2, 0.0f);

            if (versionB) {
                if (gb.bitsLeft() < 64) return false;
                union { uint32_t u; float f; } c0, c1;
                c0.u = gb.read(32);
                c1.u = gb.read(32);
                coeffs[0] = c0.f * root;
                coeffs[1] = c1.f * root;
            } else {
                if (gb.bitsLeft() < 58) return false;
                // 5 bits power, 23 bits mantissa, 1 bit sign
                auto getFloat = [&]() -> float {
                    int power = gb.read(5);
                    float f = ldexpf((float)gb.read(23), power - 23);
                    if (gb.read1()) f = -f;
                    return f;
                };
                coeffs[0] = getFloat() * root;
                coeffs[1] = getFloat() * root;
            }

            if (gb.bitsLeft() < numBands * 8) return false;

            float quant[25];
            for (int i = 0; i < numBands; i++) {
                int value = gb.read(8);
                quant[i] = quantTable[value < 96 ? value : 95];
            }

            int k = 0;
            float q = quant[0];
            int i = 2;

            while (i < frameLen) {
                int j;
                if (versionB) {
                    j = i + 16;
                } else {
                    if (gb.read1()) {
                        int v = gb.read(4);
                        j = i + rle_length_tab[v] * 8;
                    } else {
                        j = i + 8;
                    }
                }

                if (j > frameLen) j = frameLen;

                int width = gb.read(4);
                if (width == 0) {
                    for (int x = i; x < j; x++) coeffs[x] = 0.0f;
                    i = j;
                    while (k < numBands && (int)bands[k] < i)
                        q = quant[k++];
                } else {
                    while (i < j) {
                        if ((int)bands[k] == i)
                            q = quant[k++];
                        int coeff = gb.read(width);
                        if (coeff) {
                            if (gb.read1())
                                coeffs[i] = -q * coeff;
                            else
                                coeffs[i] = q * coeff;
                        } else {
                            coeffs[i] = 0.0f;
                        }
                        i++;
                    }
                }
            }

            if (useDCT) {
                coeffs[0] /= 0.5f;
                dct_inverse(coeffs.data(), out[ch], frameLen / 2);
            } else {
                rdft_inverse(coeffs.data(), frameLen);
                memcpy(out[ch], coeffs.data(), frameLen * sizeof(float));
            }
        }

        // overlap-add
        for (int ch = 0; ch < chCount; ch++) {
            int count = overlapLen * chCount;
            if (!first) {
                int j = ch;
                for (int i = 0; i < overlapLen; i++, j += chCount) {
                    out[ch][i] = (previous[ch][i] * (float)(count - j) +
                                  out[ch][i] * (float)j) / (float)count;
                }
            }
            memcpy(previous[ch], &out[ch][frameLen - overlapLen],
                   overlapLen * sizeof(float));
        }

        first = false;
        return true;
    }
};

static inline int16_t clip_int16(float v) {
    int32_t i = (int32_t)lrintf(v);
    if (i > 32767) return 32767;
    if (i < -32767) return -32767;
    return (int16_t)i;
}

bool BinkAudioDecoder::Decode1FCB(const uint8_t *data, uint32_t dataSize, Result &result) {
    if (dataSize < 20) return false;

    if (memcmp(data, "1FCB", 4) != 0) return false;

    uint8_t version = data[4];
    uint8_t channels = data[5];
    uint16_t sampleRate = data[6] | (data[7] << 8);
    uint32_t totalSamples = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
    uint32_t compressedSize = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);

    if (channels < 1 || channels > 2) return false;
    if (sampleRate == 0) return false;

    const uint8_t *binkData = data + 20;
    uint32_t binkSize = dataSize - 20;
    if (binkSize > compressedSize) binkSize = compressedSize;

    bool useDCT = (version == 'b' || version == 'B');
    bool versionB = useDCT;

    // derive frameLenBits from 1FCB block table
    // each block = one Bink Audio frame. endSample = totalSamples / numBlocks.
    // frameLen = endSample * 16/15, rounded up to power of 2.
    // for RDFT stereo: effCh=channels in derivation so channel adj in init gets correct final size.
    int derivedFrameLenBits = 0;
    if (binkSize >= 4) {
        uint16_t nb = binkData[0] | (binkData[1] << 8);
        if (nb > 0 && totalSamples > 0) {
            int effCh = useDCT ? 1 : channels;
            float samplesPerBlock = (float)totalSamples / (float)nb / (float)effCh;
            float estFrameLen = samplesPerBlock * 16.0f / 15.0f;
            int bits = 8;
            while ((1 << bits) < (int)estFrameLen && bits < 16) bits++;
            derivedFrameLenBits = bits;
        }
    }

    BinkAudioState state;
    state.init(channels, sampleRate, useDCT, versionB, derivedFrameLenBits);

    result.channels = channels;
    result.sampleRate = sampleRate;
    result.pcmData.clear();
    result.pcmData.reserve(totalSamples * channels);

    // 1FCB block table structure:
    //   uint16 numBlocks
    //   uint16 flags (typically 1)
    //   uint16 blockSizes[numBlocks]
    //   then block data (each block is self-contained coded audio)
    if (binkSize < 4) return false;
    uint16_t numBlocks = binkData[0] | (binkData[1] << 8);
    // uint16_t flags = binkData[2] | (binkData[3] << 8);

    if (numBlocks == 0 || numBlocks > 10000) return false;
    uint32_t tableStart = 4;
    uint32_t dataStart = tableStart + numBlocks * 2;
    if (dataStart > binkSize) return false;

    // two-pass: collect floats first, normalize to int16 in second pass to avoid RDFT clipping
    std::vector<float> floatSamples;
    floatSamples.reserve(totalSamples * channels);

    uint32_t blockOffset = dataStart;

    for (int blk = 0; blk < numBlocks; blk++) {
        uint16_t blockBytes = binkData[tableStart + blk * 2] |
                              (binkData[tableStart + blk * 2 + 1] << 8);
        if (blockBytes < 8 || blockOffset + blockBytes > binkSize) {
            blockOffset += blockBytes;
            continue;
        }

        BitReader gb;
        gb.init(binkData + blockOffset, blockBytes);

        std::vector<float> chBuf0(state.frameLen + 2, 0.0f);
        std::vector<float> chBuf1(state.frameLen + 2, 0.0f);
        float *outPtrs[2] = { chBuf0.data(), chBuf1.data() };

        if (!state.decodeBlock(gb, outPtrs)) {
            blockOffset += blockBytes;
            continue;
        }

        int endSample = state.frameLen - state.overlapLen;

        if (useDCT) {
            for (int i = 0; i < endSample && (int)floatSamples.size() < (int)totalSamples; i++) {
                for (int c = 0; c < channels; c++)
                    floatSamples.push_back(outPtrs[c][i]);
            }
        } else {
            for (int i = 0; i < endSample && (int)floatSamples.size() < (int)totalSamples; i++)
                floatSamples.push_back(outPtrs[0][i]);
        }

        blockOffset += blockBytes;
    }

    if (floatSamples.empty()) return false;

    // peak-normalize to int16 with ~1dB headroom
    float peak = 0;
    for (float s : floatSamples) {
        float a = fabsf(s);
        if (a > peak) peak = a;
    }

    float gain = (peak > 0) ? 30000.0f / peak : 1.0f;
    if (gain > 1.0f) gain = 1.0f; // don't amplify quiet sounds

    result.pcmData.resize(floatSamples.size());
    for (size_t i = 0; i < floatSamples.size(); i++)
        result.pcmData[i] = clip_int16(floatSamples[i] * gain);

    return true;
}

bool BinkAudioDecoder::WriteWAV(const char *path, const Result &result) {
    if (result.pcmData.empty()) return false;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint32_t pcmSize = (uint32_t)(result.pcmData.size() * sizeof(int16_t));
    uint16_t ch = (uint16_t)result.channels;
    uint32_t sr = (uint32_t)result.sampleRate;
    uint16_t bitsPerSample = 16;
    uint16_t blockAlign = ch * bitsPerSample / 8;
    uint32_t byteRate = sr * blockAlign;
    uint32_t totalSize = 36 + pcmSize;

    fwrite("RIFF", 1, 4, f);
    fwrite(&totalSize, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    uint32_t fmtSize = 16;
    fwrite(&fmtSize, 4, 1, f);
    uint16_t audioFormat = 1; // PCM
    fwrite(&audioFormat, 2, 1, f);
    fwrite(&ch, 2, 1, f);
    fwrite(&sr, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&blockAlign, 2, 1, f);
    fwrite(&bitsPerSample, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&pcmSize, 4, 1, f);
    fwrite(result.pcmData.data(), sizeof(int16_t), result.pcmData.size(), f);

    fclose(f);
    return true;
}
