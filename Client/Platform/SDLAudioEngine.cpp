#include "SDLAudioEngine.h"
#include <SDL3/SDL.h>
#include <cstring>
#include <cmath>

bool SDLAudioEngine::Init(const char *soundDir) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        return false;

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;

    m_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!m_device) return false;

    m_deviceSpec = spec;
    m_soundDir = soundDir ? soundDir : "Sound";
    memset(m_activeSounds, 0, sizeof(m_activeSounds));
    memset(m_musicStreams, 0, sizeof(m_musicStreams));

    return true;
}

void SDLAudioEngine::Shutdown() {
    StopAllSounds();
    for (auto &s : m_sounds) {
        if (s.buffer) SDL_free(s.buffer);
    }
    m_sounds.clear();
    m_soundNameToIndex.clear();

    if (m_device) {
        SDL_CloseAudioDevice(m_device);
        m_device = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

float SDLAudioEngine::LinearFalloff(float distance, float maxDist) {
    if (maxDist == 10000.0f) return 1.0f;
    if (maxDist <= 0.0f) return 0.0f;
    float atten = 1.0f - (distance / maxDist);
    if (atten < 0.0f) atten = 0.0f;
    if (atten > 1.0f) atten = 1.0f;
    return atten;
}

bool SDLAudioEngine::LoadSoundBank(const char *directory) {
    // on-demand loading
    return true;
}

int SDLAudioEngine::GetSoundIndex(const char *name) {
    auto it = m_soundNameToIndex.find(name);
    if (it != m_soundNameToIndex.end())
        return it->second;

    // miles uses "Minecraft/step/grass1" format, map to "Sound/step_grass1.wav"
    std::string sname(name);
    const char *prefix = "Minecraft/";
    size_t prefixLen = strlen(prefix);
    if (sname.compare(0, prefixLen, prefix) == 0)
        sname = sname.substr(prefixLen);

    for (auto &c : sname) {
        if (c == '/') c = '_';
    }

    std::string filepath = m_soundDir + "/" + sname + ".wav";

    SDL_AudioSpec spec;
    Uint8 *buffer = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(filepath.c_str(), &spec, &buffer, &length)) {
        SDL_Log("SDLAudioEngine: Failed to load WAV '%s': %s", filepath.c_str(), SDL_GetError());
        return -1;
    }

    int index = (int)m_sounds.size();
    SoundData sd;
    sd.name = name;
    sd.buffer = buffer;
    sd.length = length;
    sd.spec = spec;
    m_sounds.push_back(sd);
    m_soundNameToIndex[name] = index;
    return index;
}

int SDLAudioEngine::PlaySound(int soundIndex, float volume, float pitch, bool is3D,
                               float x, float y, float z) {
    if (soundIndex < 0 || soundIndex >= (int)m_sounds.size())
        return -1;

    int slot = -1;
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (!m_activeSounds[i].playing) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;

    SoundData &sd = m_sounds[soundIndex];

    SDL_AudioStream *stream = SDL_CreateAudioStream(&sd.spec, &m_deviceSpec);
    if (!stream) return -1;

    if (!SDL_PutAudioStreamData(stream, sd.buffer, sd.length)) {
        SDL_DestroyAudioStream(stream);
        return -1;
    }
    SDL_FlushAudioStream(stream);

    if (!SDL_BindAudioStream(m_device, stream)) {
        SDL_DestroyAudioStream(stream);
        return -1;
    }

    SDL_SetAudioStreamGain(stream, volume);

    SDL_AudioSound &as = m_activeSounds[slot];
    as.stream = stream;
    as.volume = volume;
    as.pitch = pitch;
    as.x = x;
    as.y = y;
    as.z = z;
    as.maxDist = 10000.0f;
    as.is3D = is3D;
    as.playing = true;
    as.looping = false;

    return slot;
}

void SDLAudioEngine::StopSound(int handle) {
    if (handle < 0 || handle >= MAX_ACTIVE_SOUNDS) return;
    SDL_AudioSound &as = m_activeSounds[handle];
    if (as.playing && as.stream) {
        SDL_UnbindAudioStream(as.stream);
        SDL_DestroyAudioStream(as.stream);
        as.stream = nullptr;
    }
    as.playing = false;
}

void SDLAudioEngine::StopAllSounds() {
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (m_activeSounds[i].playing && m_activeSounds[i].stream) {
            SDL_UnbindAudioStream(m_activeSounds[i].stream);
            SDL_DestroyAudioStream(m_activeSounds[i].stream);
            m_activeSounds[i].stream = nullptr;
        }
        m_activeSounds[i].playing = false;
    }
    for (int i = 0; i < MAX_MUSIC_STREAMS; i++) {
        if (m_musicStreams[i].stream) {
            SDL_UnbindAudioStream(m_musicStreams[i].stream);
            SDL_DestroyAudioStream(m_musicStreams[i].stream);
            m_musicStreams[i].stream = nullptr;
        }
        m_musicStreams[i].playing = false;
    }
}

void SDLAudioEngine::SetListenerPosition(float x, float y, float z) {
    m_listenerX = x; m_listenerY = y; m_listenerZ = z;
}

void SDLAudioEngine::SetListenerOrientation(float faceX, float faceY, float faceZ,
                                             float upX, float upY, float upZ) {
    m_listenerFaceX = faceX; m_listenerFaceY = faceY; m_listenerFaceZ = faceZ;
}

void SDLAudioEngine::SetSoundPosition(int handle, float x, float y, float z) {
    if (handle < 0 || handle >= MAX_ACTIVE_SOUNDS) return;
    SDL_AudioSound &as = m_activeSounds[handle];
    if (!as.playing) return;
    as.x = x; as.y = y; as.z = z;
}

void SDLAudioEngine::SetSoundVolume(int handle, float left, float right) {
    if (handle < 0 || handle >= MAX_ACTIVE_SOUNDS) return;
    SDL_AudioSound &as = m_activeSounds[handle];
    if (!as.playing || !as.stream) return;
    float vol = (left + right) * 0.5f;
    as.volume = vol;
    SDL_SetAudioStreamGain(as.stream, vol);
}

void SDLAudioEngine::SetSoundPitch(int handle, float factor) {
    if (handle < 0 || handle >= MAX_ACTIVE_SOUNDS) return;
    SDL_AudioSound &as = m_activeSounds[handle];
    if (!as.playing) return;
    as.pitch = factor;
}

void SDLAudioEngine::SetSound3DDistances(int handle, float maxDist, float minDist) {
    if (handle < 0 || handle >= MAX_ACTIVE_SOUNDS) return;
    SDL_AudioSound &as = m_activeSounds[handle];
    if (!as.playing) return;
    as.maxDist = maxDist;
}

void SDLAudioEngine::Tick(float masterSfxVolume, float masterMusicVolume) {
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        SDL_AudioSound &as = m_activeSounds[i];
        if (!as.playing || !as.stream) continue;

        if (SDL_GetAudioStreamAvailable(as.stream) == 0) {
            SDL_UnbindAudioStream(as.stream);
            SDL_DestroyAudioStream(as.stream);
            as.stream = nullptr;
            as.playing = false;
            continue;
        }

        if (as.is3D) {
            float dx = as.x - m_listenerX;
            float dy = as.y - m_listenerY;
            float dz = as.z - m_listenerZ;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            float atten = LinearFalloff(dist, as.maxDist);
            SDL_SetAudioStreamGain(as.stream, as.volume * atten * masterSfxVolume);
        } else {
            SDL_SetAudioStreamGain(as.stream, as.volume * masterSfxVolume);
        }
    }

    for (int i = 0; i < MAX_MUSIC_STREAMS; i++) {
        SDL_AudioStream_Music &ms = m_musicStreams[i];
        if (!ms.playing || !ms.stream) continue;
        SDL_SetAudioStreamGain(ms.stream, ms.volume * masterMusicVolume);
    }
}

int SDLAudioEngine::PlayMusic(const char *filepath, float volume, bool loop) {
    int slot = -1;
    for (int i = 0; i < MAX_MUSIC_STREAMS; i++) {
        if (!m_musicStreams[i].playing) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;

    std::string path(filepath);
    size_t extPos = path.rfind(".binka");
    if (extPos != std::string::npos) {
        path = path.substr(0, extPos) + ".wav";
    }

    SDL_AudioSpec spec;
    Uint8 *buffer = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.c_str(), &spec, &buffer, &length)) {
        SDL_Log("SDLAudioEngine: Failed to load music WAV '%s': %s", path.c_str(), SDL_GetError());
        return -1;
    }

    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, &m_deviceSpec);
    if (!stream) {
        SDL_free(buffer);
        return -1;
    }

    if (!SDL_PutAudioStreamData(stream, buffer, length)) {
        SDL_DestroyAudioStream(stream);
        SDL_free(buffer);
        return -1;
    }
    SDL_FlushAudioStream(stream);
    SDL_free(buffer); // Stream has a copy, free the original

    if (!SDL_BindAudioStream(m_device, stream)) {
        SDL_DestroyAudioStream(stream);
        return -1;
    }

    SDL_SetAudioStreamGain(stream, volume);

    SDL_AudioStream_Music &ms = m_musicStreams[slot];
    ms.stream = stream;
    ms.playing = true;
    ms.looping = loop;
    ms.volume = volume;

    return slot;
}

void SDLAudioEngine::StopMusic(int handle) {
    if (handle < 0 || handle >= MAX_MUSIC_STREAMS) return;
    SDL_AudioStream_Music &ms = m_musicStreams[handle];
    if (ms.stream) {
        SDL_UnbindAudioStream(ms.stream);
        SDL_DestroyAudioStream(ms.stream);
        ms.stream = nullptr;
    }
    ms.playing = false;
}

void SDLAudioEngine::SetMusicVolume(int handle, float volume) {
    if (handle < 0 || handle >= MAX_MUSIC_STREAMS) return;
    SDL_AudioStream_Music &ms = m_musicStreams[handle];
    if (!ms.playing || !ms.stream) return;
    ms.volume = volume;
    SDL_SetAudioStreamGain(ms.stream, volume);
}

bool SDLAudioEngine::IsMusicFinished(int handle) {
    if (handle < 0 || handle >= MAX_MUSIC_STREAMS) return true;
    SDL_AudioStream_Music &ms = m_musicStreams[handle];
    if (!ms.playing || !ms.stream) return true;
    return SDL_GetAudioStreamAvailable(ms.stream) == 0;
}
