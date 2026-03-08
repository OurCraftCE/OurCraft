#include "stdafx.h"

#include "SoundEngine.h"
#include "..\Consoles_App.h"
#include "..\..\Game/MultiPlayerLocalPlayer.h"
#include "..\..\..\World\FwdHeaders/net.minecraft.world.level.h"
#include "..\..\World\Level\LevelData.h"
#include "..\..\World\Math\Mth.h"
#include "../../Textures/TexturePackRepository.h"
#include "../../Textures/DLCTexturePack.h"
#include "Common\DLC\DLCAudioFile.h"


#include "..\..\Client\Windows64\Windows64_App.h"

char SoundEngine::m_szSoundPath[]={"Sound/"};
char SoundEngine::m_szMusicPath[]={"music/"};
char SoundEngine::m_szRedistName[]={"redist64"};

char *SoundEngine::m_szStreamFileA[eStream_Max]=
{
	"calm1",
	"calm2",
	"calm3",
	"hal1",
	"hal2",
	"hal3",
	"hal4",
	"nuance1",
	"nuance2",
	// add the new music tracks
	"creative1",
	"creative2",
	"creative3",
	"creative4",
	"creative5",
	"creative6",
	"menu1",
	"menu2",
	"menu3",
	"menu4",
	"piano1",
	"piano2",
	"piano3",

	// Nether
	"nether1",
	"nether2",
	"nether3",
	"nether4",
	// The End
	"the_end_dragon_alive",
	"the_end_end",
	// CDs
	"11",
	"13",
	"blocks",
	"cat",
	"chirp",
	"far",
	"mall",
	"mellohi",
	"stal",
	"strad",
	"ward",
	"where_are_we_now"
};


/////////////////////////////////////////////
//
//	init
//
/////////////////////////////////////////////
void SoundEngine::init(Options *pOptions)
{
	app.DebugPrintf("---SoundEngine::init\n");

	if (!m_sdlAudio.Init(m_szSoundPath))
	{
		app.DebugPrintf("Couldn't init SDL audio engine.\n");
		return;
	}
	app.DebugPrintf("---SoundEngine::init - SDL audio engine initialized\n");

	m_MasterMusicVolume=1.0f;
	m_MasterEffectsVolume=1.0f;

	m_bSystemMusicPlaying = false;
}


void SoundEngine::SetStreamingSounds(int iOverworldMin, int iOverWorldMax, int iNetherMin, int iNetherMax, int iEndMin, int iEndMax, int iCD1)
{
	m_iStream_Overworld_Min=iOverworldMin;
	m_iStream_Overworld_Max=iOverWorldMax;
	m_iStream_Nether_Min=iNetherMin;
	m_iStream_Nether_Max=iNetherMax;
	m_iStream_End_Min=iEndMin;
	m_iStream_End_Max=iEndMax;
	m_iStream_CD_1=iCD1;

	// array to monitor recently played tracks
	if(m_bHeardTrackA)
	{
		delete [] m_bHeardTrackA;
	}
	m_bHeardTrackA = new bool[iEndMax+1];
	memset(m_bHeardTrackA,0,sizeof(bool)*iEndMax+1);
}

// Update listener positions and call SDLAudioEngine::Tick
void SoundEngine::updateAudio()
{
	if( m_validListenerCount == 1 )
	{
		for( int i = 0; i < MAX_LOCAL_PLAYERS; i++ )
		{
			// set the listener as the first player we find
			if( m_ListenerA[i].bValid )
			{
				m_sdlAudio.SetListenerPosition(m_ListenerA[i].vPosition.x, m_ListenerA[i].vPosition.y, m_ListenerA[i].vPosition.z);
				m_sdlAudio.SetListenerOrientation(-m_ListenerA[i].vOrientFront.x, m_ListenerA[i].vOrientFront.y, m_ListenerA[i].vOrientFront.z, 0, 1, 0);
				break;
			}
		}
	}
	else
	{
		// 4J-PB - special case for splitscreen
		// the shortest distance between any listener and a sound will be used to play a sound a set distance away down the z axis.
		// The listener position will be set to 0,0,0, and the orientation will be facing down the z axis
		m_sdlAudio.SetListenerPosition(0, 0, 0);
		m_sdlAudio.SetListenerOrientation(0, 0, 1, 0, 1, 0);
	}

	m_sdlAudio.Tick(m_MasterEffectsVolume, getMasterMusicVolume());
}

/////////////////////////////////////////////
//
//	tick
//
/////////////////////////////////////////////


void SoundEngine::tick(shared_ptr<Mob> *players, float a)
{
	// update the listener positions
	int listenerCount = 0;
	if( players )
	{
		bool bListenerPostionSet=false;
		for( int i = 0; i < MAX_LOCAL_PLAYERS; i++ )
		{
			if( players[i] != NULL )
			{
				m_ListenerA[i].bValid=true;
				float x,y,z;
				x=(float)(players[i]->xo + (players[i]->x - players[i]->xo) * a);
				y=(float)(players[i]->yo + (players[i]->y - players[i]->yo) * a);
				z=(float)(players[i]->zo + (players[i]->z - players[i]->zo) * a);

				float yRot = players[i]->yRotO + (players[i]->yRot - players[i]->yRotO) * a;
				float yCos = (float)cos(-yRot * Mth::RAD_TO_GRAD - PI);
				float ySin = (float)sin(-yRot * Mth::RAD_TO_GRAD - PI);

				// store the listener positions for splitscreen
				m_ListenerA[i].vPosition.x = x;
				m_ListenerA[i].vPosition.y = y;
				m_ListenerA[i].vPosition.z = z;

				m_ListenerA[i].vOrientFront.x = ySin;
				m_ListenerA[i].vOrientFront.y = 0;
				m_ListenerA[i].vOrientFront.z = yCos;

				listenerCount++;
			}
			else
			{
				m_ListenerA[i].bValid=false;
			}
		}
	}


	// If there were no valid players set, make up a default listener
	if( listenerCount == 0 )
	{
		m_ListenerA[0].vPosition.x = 0;
		m_ListenerA[0].vPosition.y = 0;
		m_ListenerA[0].vPosition.z = 0;
		m_ListenerA[0].vOrientFront.x = 0;
		m_ListenerA[0].vOrientFront.y = 0;
		m_ListenerA[0].vOrientFront.z = 1.0f;
		listenerCount++;
	}
	m_validListenerCount = listenerCount;

	updateAudio();
}

/////////////////////////////////////////////
//
//	SoundEngine
//
/////////////////////////////////////////////
SoundEngine::SoundEngine()
{
	random = new Random();
	m_musicHandle = -1;
	m_StreamState=eMusicStreamState_Idle;
	m_iMusicDelay=0;
	m_validListenerCount=0;

	m_bHeardTrackA=NULL;

	// Start the streaming music playing some music from the overworld
	SetStreamingSounds(eStream_Overworld_Calm1,eStream_Overworld_piano3,
		eStream_Nether1,eStream_Nether4,
		eStream_end_dragon,eStream_end_end,
		eStream_CD_1);

	m_musicID=getMusicID(LevelData::DIMENSION_OVERWORLD);

	m_StreamingAudioInfo.bIs3D=false;
	m_StreamingAudioInfo.x=0;
	m_StreamingAudioInfo.y=0;
	m_StreamingAudioInfo.z=0;
	m_StreamingAudioInfo.volume=1;
	m_StreamingAudioInfo.pitch=1;

	memset(CurrentSoundsPlaying,0,sizeof(int)*(eSoundType_MAX+eSFX_MAX));
	memset(m_ListenerA,0,sizeof(AUDIO_LISTENER)*XUSER_MAX_COUNT);

}

void SoundEngine::destroy()
{
	m_sdlAudio.Shutdown();
}

#ifdef _DEBUG
void SoundEngine::GetSoundName(char *szSoundName,int iSound)
{
	strcpy((char *)szSoundName,"Minecraft/");
	wstring name = wchSoundNames[iSound];
	char *SoundName = (char *)ConvertSoundPathToName(name);
	strcat((char *)szSoundName,SoundName);
}
#endif // _DEBUG

/////////////////////////////////////////////
//
//	play
//
/////////////////////////////////////////////
void SoundEngine::play(int iSound, float x, float y, float z, float volume, float pitch)
{
	char szSoundName[256];

	if(iSound==-1)
	{
		app.DebugPrintf(6,"PlaySound with sound of -1 !!!!!!!!!!!!!!!\n");
		return;
	}

	// build the name
	wstring name = wchSoundNames[iSound];
	char *SoundName = (char *)ConvertSoundPathToName(name);
	snprintf(szSoundName, sizeof(szSoundName), "Minecraft/%s", SoundName);

//	app.DebugPrintf(6,"PlaySound - %d - %s - %s (%f %f %f, vol %f, pitch %f)\n",iSound, SoundName, szSoundName,x,y,z,volume,pitch);

	// Clamp volume
	if(volume > 1.0f) volume = 1.0f;

	int idx = m_sdlAudio.GetSoundIndex(szSoundName);
	if (idx >= 0)
	{
		float distanceScaler = 16.0f;

		// Check for dragon/ghast/thunder distance scalers
		bool isThunder = (volume == 10000.0f);
		if (isThunder) volume = 1.0f;

		switch(iSound)
		{
			// Is this the Dragon?
			case eSoundType_MOB_ENDERDRAGON_GROWL:
			case eSoundType_MOB_ENDERDRAGON_MOVE:
			case eSoundType_MOB_ENDERDRAGON_END:
			case eSoundType_MOB_ENDERDRAGON_HIT:
				distanceScaler=100.0f;
				break;
			case eSoundType_MOB_GHAST_MOAN:
			case eSoundType_MOB_GHAST_SCREAM:
			case eSoundType_MOB_GHAST_DEATH:
			case eSoundType_MOB_GHAST_CHARGE:
			case eSoundType_MOB_GHAST_FIREBALL:
				distanceScaler=30.0f;
				break;
		}

		// Set a special distance scaler for thunder, which we respond to by having no attenuation
		if(isThunder)
		{
			distanceScaler = 10000.0f;
		}

		// For splitscreen, calculate distance to nearest listener and position sound accordingly
		float playX = x, playY = y, playZ = z;
		if(m_validListenerCount > 1)
		{
			float fClosest=10000.0f;
			float fClosestX=0.0f,fClosestY=0.0f,fClosestZ=0.0f,fDist;
			for( int i = 0; i < MAX_LOCAL_PLAYERS; i++ )
			{
				if( m_ListenerA[i].bValid )
				{
					float dx=fabs(m_ListenerA[i].vPosition.x-x);
					float dy=fabs(m_ListenerA[i].vPosition.y-y);
					float dz=fabs(m_ListenerA[i].vPosition.z-z);
					fDist=dx+dy+dz;
					if(fDist<fClosest)
					{
						fClosest=fDist;
						fClosestX=dx;
						fClosestY=dy;
						fClosestZ=dz;
					}
				}
			}
			fDist=sqrtf((fClosestX*fClosestX)+(fClosestY*fClosestY)+(fClosestZ*fClosestZ));
			playX = 0; playY = 0; playZ = fDist;
		}

		int handle = m_sdlAudio.PlaySound(idx, volume, pitch, true, playX, playY, playZ);
		if (handle >= 0)
		{
			m_sdlAudio.SetSound3DDistances(handle, distanceScaler, 1.0f);
			if(!pitch) // bUseSoundsPitchVal was false in play(), so always set pitch
			{
				m_sdlAudio.SetSoundPitch(handle, pitch);
			}
		}
	}
}

/////////////////////////////////////////////
//
//	playUI
//
/////////////////////////////////////////////
void SoundEngine::playUI(int iSound, float volume, float pitch)
{
	char szSoundName[256];
	wstring name;
	// we have some game sounds played as UI sounds...
	// Not the best way to do this, but it seems to only be the portal sounds

	const char *prefix;
	if(iSound>=eSFX_MAX)
	{
		prefix = "Minecraft/";
		name = wchSoundNames[iSound];
	}
	else
	{
		prefix = "Minecraft/UI/";
		name = wchUISoundNames[iSound];
	}

	char *SoundName = (char *)ConvertSoundPathToName(name);
	snprintf(szSoundName, sizeof(szSoundName), "%s%s", prefix, SoundName);
//	app.DebugPrintf("UI: Playing %s, volume %f, pitch %f\n",SoundName,volume,pitch);

	int idx = m_sdlAudio.GetSoundIndex(szSoundName);
	if (idx >= 0)
	{
		m_sdlAudio.PlaySound(idx, volume, pitch, false, 0, 0, 0);
	}
}

/////////////////////////////////////////////
//
//	playStreaming
//
/////////////////////////////////////////////
void SoundEngine::playStreaming(const wstring& name, float x, float y , float z, float volume, float pitch, bool bMusicDelay)
{
	// This function doesn't actually play a streaming sound, just sets states and an id for the music tick to play it
	// Level audio will be played when a play with an empty name comes in
	// CD audio will be played when a named stream comes in

	m_StreamingAudioInfo.x=x;
	m_StreamingAudioInfo.y=y;
	m_StreamingAudioInfo.z=z;
	m_StreamingAudioInfo.volume=volume;
	m_StreamingAudioInfo.pitch=pitch;

	if(m_StreamState==eMusicStreamState_Playing)
	{
		m_StreamState=eMusicStreamState_Stop;
	}

	if(name.empty())
	{
		// music, or stop CD
		m_StreamingAudioInfo.bIs3D=false;

		// we need a music id
		// random delay of up to 3 minutes for music
		m_iMusicDelay = random->nextInt(20 * 60 * 3);//random->nextInt(20 * 60 * 10) + 20 * 60 * 10;

#ifdef _DEBUG
		m_iMusicDelay=0;
#endif
		Minecraft *pMinecraft=Minecraft::GetInstance();

		bool playerInEnd=false;
		bool playerInNether=false;

		for(unsigned int i=0;i<MAX_LOCAL_PLAYERS;i++)
		{
			if(pMinecraft->localplayers[i]!=NULL)
			{
				if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_END)
				{
					playerInEnd=true;
				}
				else if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_NETHER)
				{
					playerInNether=true;
				}
			}
		}
		if(playerInEnd)
		{
			m_musicID = getMusicID(LevelData::DIMENSION_END);
		}
		else if(playerInNether)
		{
			m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
		}
		else
		{
			m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
		}
	}
	else
	{
		// jukebox
		m_StreamingAudioInfo.bIs3D=true;
		m_musicID=getMusicID(name);
		m_iMusicDelay=0;
	}
}


int SoundEngine::GetRandomishTrack(int iStart,int iEnd)
{
	// 4J-PB - make it more likely that we'll get a track we've not heard for a while, although repeating tracks sometimes is fine

	// if all tracks have been heard, clear the flags
	bool bAllTracksHeard=true;
	int iVal=iStart;
	for(int i=iStart;i<=iEnd;i++)
	{
		if(m_bHeardTrackA[i]==false)
		{
			bAllTracksHeard=false;
			app.DebugPrintf("Not heard all tracks yet\n");
			break;
		}
	}

	if(bAllTracksHeard)
	{
		app.DebugPrintf("Heard all tracks - resetting the tracking array\n");

		for(int i=iStart;i<=iEnd;i++)
		{
			m_bHeardTrackA[i]=false;
		}
	}

	// trying to get a track we haven't heard, but not too hard
	for(int i=0;i<=((iEnd-iStart)/2);i++)
	{
		// random->nextInt(1) will always return 0
		iVal=random->nextInt((iEnd-iStart)+1)+iStart;
		if(m_bHeardTrackA[iVal]==false)
		{
			// not heard this
			app.DebugPrintf("(%d) Not heard track %d yet, so playing it now\n",i,iVal);
			m_bHeardTrackA[iVal]=true;
			break;
		}
		else
		{
			app.DebugPrintf("(%d) Skipping track %d already heard it recently\n",i,iVal);
		}
	}

	app.DebugPrintf("Select track %d\n",iVal);
	return iVal;
}
/////////////////////////////////////////////
//
//	getMusicID
//
/////////////////////////////////////////////
int SoundEngine::getMusicID(int iDomain)
{
	int iRandomVal=0;
	Minecraft *pMinecraft=Minecraft::GetInstance();

	// Before the game has started?
	if(pMinecraft==NULL)
	{
		// any track from the overworld
		return GetRandomishTrack(m_iStream_Overworld_Min,m_iStream_Overworld_Max);
	}

	if(pMinecraft->skins->isUsingDefaultSkin())
	{
		switch(iDomain)
		{
		case LevelData::DIMENSION_END:
			// the end isn't random - it has different music depending on whether the dragon is alive or not, but we've not added the dead dragon music yet
			return m_iStream_End_Min;
		case LevelData::DIMENSION_NETHER:
			return GetRandomishTrack(m_iStream_Nether_Min,m_iStream_Nether_Max);
		default: //overworld
			return GetRandomishTrack(m_iStream_Overworld_Min,m_iStream_Overworld_Max);
		}
	}
	else
	{
		// using a texture pack - may have multiple End music tracks
		switch(iDomain)
		{
		case LevelData::DIMENSION_END:
			return GetRandomishTrack(m_iStream_End_Min,m_iStream_End_Max);
		case LevelData::DIMENSION_NETHER:
			return GetRandomishTrack(m_iStream_Nether_Min,m_iStream_Nether_Max);
		default: //overworld
			return GetRandomishTrack(m_iStream_Overworld_Min,m_iStream_Overworld_Max);
		}
	}
}

/////////////////////////////////////////////
//
//	getMusicID
//
/////////////////////////////////////////////
// check what the CD is
int SoundEngine::getMusicID(const wstring& name)
{
	int iCD=0;
	char *SoundName = (char *)ConvertSoundPathToName(name,true);

	// 4J-PB - these will always be the game cds, so use the m_szStreamFileA for this
	for(int i=0;i<12;i++)
	{
		if(strcmp(SoundName,m_szStreamFileA[i+eStream_CD_1])==0)
		{
			iCD=i;
			break;
		}
	}

	// adjust for cd start position on normal or mash-up pack
	return iCD+m_iStream_CD_1;
}

/////////////////////////////////////////////
//
//	getMasterMusicVolume
//
/////////////////////////////////////////////
float SoundEngine::getMasterMusicVolume()
{
	if( m_bSystemMusicPlaying )
	{
		return 0.0f;
	}
	else
	{
		return m_MasterMusicVolume;
	}
}

/////////////////////////////////////////////
//
//	updateMusicVolume
//
/////////////////////////////////////////////
void SoundEngine::updateMusicVolume(float fVal)
{
	m_MasterMusicVolume=fVal;
}

/////////////////////////////////////////////
//
//	updateSystemMusicPlaying
//
/////////////////////////////////////////////
void SoundEngine::updateSystemMusicPlaying(bool isPlaying)
{
	m_bSystemMusicPlaying = isPlaying;
}

/////////////////////////////////////////////
//
//	updateSoundEffectVolume
//
/////////////////////////////////////////////
void SoundEngine::updateSoundEffectVolume(float fVal)
{
	m_MasterEffectsVolume=fVal;
}

void SoundEngine::add(const wstring& name, File *file) {}
void SoundEngine::addMusic(const wstring& name, File *file) {}
void SoundEngine::addStreaming(const wstring& name, File *file) {}
bool SoundEngine::isStreamingWavebankReady() { return true; }

/////////////////////////////////////////////
//
//	playMusicTick
//
/////////////////////////////////////////////
void SoundEngine::playMusicTick()
{
// AP - vita will update the music during the mixer callback
	playMusicUpdate();
}

// Music state machine using SDLAudioEngine
void SoundEngine::playMusicUpdate()
{
	static float fMusicVol = 0.0f;
	static bool firstCall = true;
	if( firstCall )
	{
		fMusicVol = getMasterMusicVolume();
		firstCall = false;
	}

	switch(m_StreamState)
	{
	case eMusicStreamState_Idle:

		// start a stream playing
		if (m_iMusicDelay > 0)
		{
			m_iMusicDelay--;
			return;
		}

		if(m_musicID!=-1)
		{
			// start playing it
			snprintf(m_szStreamName, sizeof(m_szStreamName), "%s/%s", getUsrDirPath(), m_szMusicPath);

			strcpy((char *)m_szStreamName,m_szMusicPath);
			// are we using a mash-up pack?
			if(Minecraft::GetInstance()->skins->getSelected()->hasAudio())
			{
				// It's a mash-up - need to use the DLC path for the music
				TexturePack *pTexPack=Minecraft::GetInstance()->skins->getSelected();
				DLCTexturePack *pDLCTexPack=(DLCTexturePack *)pTexPack;
				DLCPack *pack = pDLCTexPack->getDLCInfoParentPack();
				DLCAudioFile *dlcAudioFile = (DLCAudioFile *) pack->getFile(DLCManager::e_DLCType_Audio, 0);

				app.DebugPrintf("Mashup pack \n");

				// build the name
				// if the music ID is beyond the end of the texture pack music files, then it's a CD
				if(m_musicID<m_iStream_CD_1)
				{
					SetIsPlayingStreamingGameMusic(true);
					SetIsPlayingStreamingCDMusic(false);
					m_MusicType=eMusicType_Game;
					m_StreamingAudioInfo.bIs3D=false;

					wstring &wstrSoundName=dlcAudioFile->GetSoundName(m_musicID);
					wstring wstrFile=L"TPACK:\\Data\\" + wstrSoundName +L".binka";
					std::wstring mountedPath = StorageManager.GetMountedPath(wstrFile);
					wcstombs(m_szStreamName,mountedPath.c_str(),255);
				}
				else
				{
					SetIsPlayingStreamingGameMusic(false);
					SetIsPlayingStreamingCDMusic(true);
					m_MusicType=eMusicType_CD;
					m_StreamingAudioInfo.bIs3D=true;

					// Need to adjust to index into the cds in the game's m_szStreamFileA
					strncat((char *)m_szStreamName,"cds/", sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
					strncat((char *)m_szStreamName,m_szStreamFileA[m_musicID-m_iStream_CD_1+eStream_CD_1], sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
					strncat((char *)m_szStreamName,".binka", sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
				}
			}
			else
			{
				if(m_musicID<m_iStream_CD_1)
				{
					SetIsPlayingStreamingGameMusic(true);
					SetIsPlayingStreamingCDMusic(false);
					m_MusicType=eMusicType_Game;
					m_StreamingAudioInfo.bIs3D=false;
					// build the name
					strncat((char *)m_szStreamName,"music/", sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
				}
				else
				{
					SetIsPlayingStreamingGameMusic(false);
					SetIsPlayingStreamingCDMusic(true);
					m_MusicType=eMusicType_CD;
					m_StreamingAudioInfo.bIs3D=true;
					// build the name
					strncat((char *)m_szStreamName,"cds/", sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
				}
				strncat((char *)m_szStreamName,m_szStreamFileA[m_musicID], sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);
				strncat((char *)m_szStreamName,".binka", sizeof(m_szStreamName) - strlen(m_szStreamName) - 1);

			}

			app.DebugPrintf("Starting streaming - %s\n",m_szStreamName);

			// PlayMusic is synchronous (loads WAV), no thread needed
			m_musicHandle = m_sdlAudio.PlayMusic(m_szStreamName, m_StreamingAudioInfo.volume * getMasterMusicVolume(), false);
			if (m_musicHandle >= 0)
			{
				m_StreamState = eMusicStreamState_Playing;
			}
			else
			{
				app.DebugPrintf("Failed to start music: %s\n", m_szStreamName);
				// Try again later
				m_StreamState = eMusicStreamState_Completed;
			}
		}
		break;

	case eMusicStreamState_Stop:
		// stop the music
		if (m_musicHandle >= 0)
		{
			m_sdlAudio.StopMusic(m_musicHandle);
			m_musicHandle = -1;
		}
		SetIsPlayingStreamingCDMusic(false);
		SetIsPlayingStreamingGameMusic(false);
		m_StreamState=eMusicStreamState_Idle;
		break;

	case eMusicStreamState_Stopping:
		break;

	case eMusicStreamState_Playing:
		if(GetIsPlayingStreamingGameMusic())
		{
			{
				bool playerInEnd = false;
				bool playerInNether=false;
				Minecraft *pMinecraft = Minecraft::GetInstance();
				for(unsigned int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
				{
					if(pMinecraft->localplayers[i]!=NULL)
					{
						if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_END)
						{
							playerInEnd=true;
						}
						else if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_NETHER)
						{
							playerInNether=true;
						}
					}
				}

				if(playerInEnd && !GetIsPlayingEndMusic())
				{
					m_StreamState=eMusicStreamState_Stop;

					// Set the end track
					m_musicID = getMusicID(LevelData::DIMENSION_END);
					SetIsPlayingEndMusic(true);
					SetIsPlayingNetherMusic(false);
				}
				else if(!playerInEnd && GetIsPlayingEndMusic())
				{
					if(playerInNether)
					{
						m_StreamState=eMusicStreamState_Stop;

						// Set the end track
						m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
						SetIsPlayingEndMusic(false);
						SetIsPlayingNetherMusic(true);
					}
					else
					{
						m_StreamState=eMusicStreamState_Stop;

						// Set the end track
						m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
						SetIsPlayingEndMusic(false);
						SetIsPlayingNetherMusic(false);
					}
				}
				else if (playerInNether && !GetIsPlayingNetherMusic())
				{
					m_StreamState=eMusicStreamState_Stop;
					// set the Nether track
					m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
					SetIsPlayingNetherMusic(true);
					SetIsPlayingEndMusic(false);
				}
				else if(!playerInNether && GetIsPlayingNetherMusic())
				{
					if(playerInEnd)
					{
						m_StreamState=eMusicStreamState_Stop;
						// set the Nether track
						m_musicID = getMusicID(LevelData::DIMENSION_END);
						SetIsPlayingNetherMusic(false);
						SetIsPlayingEndMusic(true);
					}
					else
					{
						m_StreamState=eMusicStreamState_Stop;
						// set the Nether track
						m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
						SetIsPlayingNetherMusic(false);
						SetIsPlayingEndMusic(false);
					}
				}

				// volume change required?
				if(fMusicVol!=getMasterMusicVolume())
				{
					fMusicVol=getMasterMusicVolume();
					if (m_musicHandle >= 0)
					{
						m_sdlAudio.SetMusicVolume(m_musicHandle, fMusicVol);
					}
				}
			}
		}

		break;

	case eMusicStreamState_Completed:
		{
			// random delay of up to 3 minutes for music
			m_iMusicDelay = random->nextInt(20 * 60 * 3);//random->nextInt(20 * 60 * 10) + 20 * 60 * 10;
			// Check if we have a local player in The Nether or in The End, and play that music if they are
			Minecraft *pMinecraft=Minecraft::GetInstance();
			bool playerInEnd=false;
			bool playerInNether=false;

			for(unsigned int i=0;i<MAX_LOCAL_PLAYERS;i++)
			{
				if(pMinecraft->localplayers[i]!=NULL)
				{
					if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_END)
					{
						playerInEnd=true;
					}
					else if(pMinecraft->localplayers[i]->dimension==LevelData::DIMENSION_NETHER)
					{
						playerInNether=true;
					}
				}
			}
			if(playerInEnd)
			{
				m_musicID = getMusicID(LevelData::DIMENSION_END);
				SetIsPlayingEndMusic(true);
				SetIsPlayingNetherMusic(false);
			}
			else if(playerInNether)
			{
				m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
				SetIsPlayingNetherMusic(true);
				SetIsPlayingEndMusic(false);
			}
			else
			{
				m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
				SetIsPlayingNetherMusic(false);
				SetIsPlayingEndMusic(false);
			}

			m_StreamState=eMusicStreamState_Idle;
		}
		break;
	}

	// check the status of the music - this is for when a track completes rather than is stopped by the user action
	if(m_musicHandle >= 0)
	{
		if(m_sdlAudio.IsMusicFinished(m_musicHandle))
		{
			m_sdlAudio.StopMusic(m_musicHandle);
			m_musicHandle = -1;
			SetIsPlayingStreamingCDMusic(false);
			SetIsPlayingStreamingGameMusic(false);

			m_StreamState=eMusicStreamState_Completed;
		}
	}
}


/////////////////////////////////////////////
//
//	ConvertSoundPathToName
//
/////////////////////////////////////////////
char *SoundEngine::ConvertSoundPathToName(const wstring& name, bool bConvertSpaces)
{
	static char buf[256];
	assert(name.length()<256);
	for(unsigned int i = 0; i < name.length(); i++ )
	{
		wchar_t c = name[i];
		if(c=='.') c='/';
		if(bConvertSpaces)
		{
			if(c==' ') c='_';
		}
		buf[i] = (char)c;
	}
	buf[name.length()] = 0;
	return buf;
}
