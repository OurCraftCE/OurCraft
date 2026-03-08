#pragma once
#include <string>
#include <windows.h>
#include "Win64SavePaths.h"
#include "..\Common\App_structs.h"

// PC replacement for console profile data (XCONTENT etc.)
// Stores GAME_SETTINGS + stats data as a binary file per player slot
class Win64ProfileData
{
public:
	static const DWORD PROFILE_VERSION = 1;
	static const int MAX_PROFILE_SLOTS = 4; // XUSER_MAX_COUNT

	// File header for profile binary format
	#pragma pack(push, 1)
	struct ProfileHeader
	{
		DWORD magic;           // 'MCE\0' = 0x0045434D
		DWORD version;         // PROFILE_VERSION
		DWORD gameSettingsSize; // sizeof(GAME_SETTINGS)
		DWORD statsDataSize;   // size of stats blob following settings
		DWORD crc32;           // CRC of data following header
	};
	#pragma pack(pop)

	Win64ProfileData();
	~Win64ProfileData();

	// Initialize profile data storage; allocates backing memory
	void Initialize(int gameDefinedDataSizePerPlayer);

	// Load profile for a given player slot (0..MAX_PROFILE_SLOTS-1)
	bool Load(int slot);

	// Save profile for a given player slot
	bool Save(int slot);

	// Get pointer to game-defined profile data for a slot
	// This returns a buffer of gameDefinedDataSize bytes
	void* GetGameDefinedData(int slot);

	// Get the GAME_SETTINGS pointer for a slot (convenience, same as above cast)
	GAME_SETTINGS* GetGameSettings(int slot);

	// Mark slot as dirty so CheckGameSettingsChanged knows to save
	void MarkDirty(int slot) { m_dirty[slot] = true; }
	bool IsDirty(int slot) const { return m_dirty[slot]; }
	void ClearDirty(int slot) { m_dirty[slot] = false; }

	// Save all dirty profiles
	void SaveAllDirty();

private:
	std::wstring GetProfilePath(int slot);
	DWORD ComputeCRC(const BYTE* data, DWORD size);

	int  m_gameDefinedDataSize;
	BYTE* m_profileData[MAX_PROFILE_SLOTS]; // allocated buffers
	bool  m_dirty[MAX_PROFILE_SLOTS];
	bool  m_initialized;
};

extern Win64ProfileData g_Win64ProfileData;
