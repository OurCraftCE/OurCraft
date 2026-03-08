#pragma once
#include <SDL3/SDL_audio.h>
#include <string>
#include <vector>
#include <unordered_map>

struct SDL_AudioSound {
    SDL_AudioStream *stream;
    float volume;
    float pitch;
    float x, y, z;
    float maxDist;
    bool is3D;
    bool playing;
    bool looping;
};

struct SDL_AudioStream_Music {
    SDL_AudioStream *stream;
    bool playing;
    bool looping;
    float volume;
};

class SDLAudioEngine {
public:
    bool Init(const char *soundDir);
    void Shutdown();

    bool LoadSoundBank(const char *directory);
    int GetSoundIndex(const char *name);

    int PlaySound(int soundIndex, float volume, float pitch, bool is3D,
                  float x, float y, float z);
    void StopSound(int handle);
    void StopAllSounds();

    void SetListenerPosition(float x, float y, float z);
    void SetListenerOrientation(float faceX, float faceY, float faceZ,
                                float upX, float upY, float upZ);
    void SetSoundPosition(int handle, float x, float y, float z);
    void SetSoundVolume(int handle, float left, float right);
    void SetSoundPitch(int handle, float factor);
    void SetSound3DDistances(int handle, float maxDist, float minDist);

    int PlayMusic(const char *filepath, float volume, bool loop);
    void StopMusic(int handle);
    void SetMusicVolume(int handle, float volume);
    bool IsMusicFinished(int handle);

    void Tick(float masterSfxVolume, float masterMusicVolume);

    static float LinearFalloff(float distance, float maxDist);

private:
    SDL_AudioDeviceID m_device = 0;
    SDL_AudioSpec m_deviceSpec;

    float m_listenerX = 0, m_listenerY = 0, m_listenerZ = 0;
    float m_listenerFaceX = 0, m_listenerFaceY = 0, m_listenerFaceZ = -1;

    struct SoundData {
        std::string name;
        Uint8 *buffer;
        Uint32 length;
        SDL_AudioSpec spec;
    };
    std::vector<SoundData> m_sounds;
    std::unordered_map<std::string, int> m_soundNameToIndex;

    static const int MAX_ACTIVE_SOUNDS = 64;
    SDL_AudioSound m_activeSounds[MAX_ACTIVE_SOUNDS];

    static const int MAX_MUSIC_STREAMS = 4;
    SDL_AudioStream_Music m_musicStreams[MAX_MUSIC_STREAMS];

    std::string m_soundDir;
};
