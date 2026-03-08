#include "stdafx.h"
#include "Win64ProfileData.h"

Win64ProfileData g_Win64ProfileData;

static const DWORD PROFILE_MAGIC = 0x0045434D; // 'MCE\0'

Win64ProfileData::Win64ProfileData()
{
	m_gameDefinedDataSize = 0;
	m_initialized = false;
	for (int i = 0; i < MAX_PROFILE_SLOTS; i++)
	{
		m_profileData[i] = NULL;
		m_dirty[i] = false;
	}
}

Win64ProfileData::~Win64ProfileData()
{
	for (int i = 0; i < MAX_PROFILE_SLOTS; i++)
	{
		if (m_profileData[i])
		{
			delete[] m_profileData[i];
			m_profileData[i] = NULL;
		}
	}
}

void Win64ProfileData::Initialize(int gameDefinedDataSizePerPlayer)
{
	m_gameDefinedDataSize = gameDefinedDataSizePerPlayer;

	for (int i = 0; i < MAX_PROFILE_SLOTS; i++)
	{
		if (m_profileData[i])
			delete[] m_profileData[i];

		m_profileData[i] = new BYTE[m_gameDefinedDataSize];
		ZeroMemory(m_profileData[i], m_gameDefinedDataSize);
		m_dirty[i] = false;
	}

	m_initialized = true;

	// Try to load existing profiles
	for (int i = 0; i < MAX_PROFILE_SLOTS; i++)
	{
		Load(i);
	}
}

void* Win64ProfileData::GetGameDefinedData(int slot)
{
	if (slot < 0 || slot >= MAX_PROFILE_SLOTS || !m_profileData[slot])
		return NULL;
	return m_profileData[slot];
}

GAME_SETTINGS* Win64ProfileData::GetGameSettings(int slot)
{
	return (GAME_SETTINGS*)GetGameDefinedData(slot);
}

std::wstring Win64ProfileData::GetProfilePath(int slot)
{
	if (!Win64SavePaths::IsInitialized())
		return L"";

	return Win64SavePaths::GetProfilesDir() + L"profile_" + std::to_wstring(slot) + L".dat";
}

DWORD Win64ProfileData::ComputeCRC(const BYTE* data, DWORD size)
{
	// Simple CRC-32 (same polynomial as zlib)
	DWORD crc = 0xFFFFFFFF;
	for (DWORD i = 0; i < size; i++)
	{
		crc ^= data[i];
		for (int j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}
	return crc ^ 0xFFFFFFFF;
}

bool Win64ProfileData::Load(int slot)
{
	if (slot < 0 || slot >= MAX_PROFILE_SLOTS || !m_initialized)
		return false;

	std::wstring path = GetProfilePath(slot);
	if (path.empty())
		return false;

	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	ProfileHeader header;
	DWORD bytesRead = 0;
	BOOL ok = ReadFile(hFile, &header, sizeof(header), &bytesRead, NULL);
	if (!ok || bytesRead != sizeof(header))
	{
		CloseHandle(hFile);
		return false;
	}

	// Validate header
	if (header.magic != PROFILE_MAGIC || header.version != PROFILE_VERSION)
	{
		CloseHandle(hFile);
		return false;
	}

	// Read the game settings data (up to our buffer size)
	DWORD dataToRead = min((DWORD)m_gameDefinedDataSize, header.gameSettingsSize + header.statsDataSize);
	ok = ReadFile(hFile, m_profileData[slot], dataToRead, &bytesRead, NULL);
	CloseHandle(hFile);

	if (!ok || bytesRead != dataToRead)
		return false;

	// Verify CRC
	DWORD crc = ComputeCRC(m_profileData[slot], dataToRead);
	if (crc != header.crc32)
	{
		// CRC mismatch - zero out and return false
		ZeroMemory(m_profileData[slot], m_gameDefinedDataSize);
		return false;
	}

	return true;
}

bool Win64ProfileData::Save(int slot)
{
	if (slot < 0 || slot >= MAX_PROFILE_SLOTS || !m_initialized)
		return false;

	std::wstring path = GetProfilePath(slot);
	if (path.empty())
		return false;

	// Atomic write: write to .tmp then rename
	std::wstring tmpPath = path + L".tmp";

	HANDLE hFile = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD gameSettingsSize = sizeof(GAME_SETTINGS);
	DWORD statsDataSize = (m_gameDefinedDataSize > (int)gameSettingsSize) ?
		(m_gameDefinedDataSize - gameSettingsSize) : 0;
	DWORD totalDataSize = gameSettingsSize + statsDataSize;

	ProfileHeader header;
	header.magic = PROFILE_MAGIC;
	header.version = PROFILE_VERSION;
	header.gameSettingsSize = gameSettingsSize;
	header.statsDataSize = statsDataSize;
	header.crc32 = ComputeCRC(m_profileData[slot], totalDataSize);

	DWORD bytesWritten = 0;
	BOOL ok = WriteFile(hFile, &header, sizeof(header), &bytesWritten, NULL);
	if (ok)
	{
		ok = WriteFile(hFile, m_profileData[slot], totalDataSize, &bytesWritten, NULL);
	}

	FlushFileBuffers(hFile);
	CloseHandle(hFile);

	if (!ok)
	{
		DeleteFileW(tmpPath.c_str());
		return false;
	}

	// Atomic rename: delete old, rename tmp
	DeleteFileW(path.c_str());
	if (!MoveFileW(tmpPath.c_str(), path.c_str()))
	{
		// If rename fails, try copy
		CopyFileW(tmpPath.c_str(), path.c_str(), FALSE);
		DeleteFileW(tmpPath.c_str());
	}

	m_dirty[slot] = false;
	return true;
}

void Win64ProfileData::SaveAllDirty()
{
	for (int i = 0; i < MAX_PROFILE_SLOTS; i++)
	{
		if (m_dirty[i])
			Save(i);
	}
}
