#pragma once

#include "..\..\..\World\Entities\SoundTypes.h"

// Types previously from Miles mss.h
typedef float F32;
typedef int S32;
typedef __int64 S64;
typedef unsigned __int64 U64;
typedef unsigned char U8;
#define AILCALLBACK
#define AILCALL

typedef struct
{
	float x,y,z;
}
AUDIO_VECTOR;

typedef struct
{
	bool bValid;
	AUDIO_VECTOR vPosition;
	AUDIO_VECTOR vOrientFront;
}
AUDIO_LISTENER;

class Options;

class ConsoleSoundEngine
{
public:

	ConsoleSoundEngine() : m_bIsPlayingStreamingCDMusic(false),m_bIsPlayingStreamingGameMusic(false), m_bIsPlayingEndMusic(false),m_bIsPlayingNetherMusic(false){};
	virtual void tick(shared_ptr<Mob> *players, float a) =0;
	virtual void destroy()=0;
	virtual void play(int iSound, float x, float y, float z, float volume, float pitch) =0;
	virtual void playStreaming(const wstring& name, float x, float y , float z, float volume, float pitch, bool bMusicDelay=true) =0;
	virtual void playUI(int iSound, float volume, float pitch) =0;
	virtual void updateMusicVolume(float fVal) =0;
	virtual void updateSystemMusicPlaying(bool isPlaying) = 0;
	virtual void updateSoundEffectVolume(float fVal) =0;
	virtual void init(Options *) =0 ;
	virtual void add(const wstring& name, File *file) =0;
	virtual void addMusic(const wstring& name, File *file) =0;
	virtual void addStreaming(const wstring& name, File *file)  =0;
	virtual char *ConvertSoundPathToName(const wstring& name, bool bConvertSpaces) =0;
	virtual void playMusicTick() =0;

	virtual bool GetIsPlayingStreamingCDMusic()				;
	virtual bool GetIsPlayingStreamingGameMusic()			;
	virtual void SetIsPlayingStreamingCDMusic(bool bVal)	;
	virtual void SetIsPlayingStreamingGameMusic(bool bVal)	;
	virtual bool GetIsPlayingEndMusic()						;
	virtual bool GetIsPlayingNetherMusic()					;
	virtual void SetIsPlayingEndMusic(bool bVal)			;
	virtual void SetIsPlayingNetherMusic(bool bVal)			;
	static const WCHAR *wchSoundNames[eSoundType_MAX];
	static const WCHAR *wchUISoundNames[eSFX_MAX];

private:
	// platform specific functions

	virtual int initAudioHardware(int iMinSpeakers)=0;

	bool m_bIsPlayingStreamingCDMusic;
	bool m_bIsPlayingStreamingGameMusic;
	bool m_bIsPlayingEndMusic;
	bool m_bIsPlayingNetherMusic;
};
