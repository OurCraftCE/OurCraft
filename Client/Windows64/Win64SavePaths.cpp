#include "stdafx.h"
#include "Win64SavePaths.h"
#include <shlobj.h>

bool         Win64SavePaths::s_initialized = false;
std::wstring Win64SavePaths::s_baseDir;
std::wstring Win64SavePaths::s_settingsDir;
std::wstring Win64SavePaths::s_worldSavesDir;
std::wstring Win64SavePaths::s_profilesDir;
std::wstring Win64SavePaths::s_dlcDir;
std::wstring Win64SavePaths::s_screenshotsDir;

bool Win64SavePaths::EnsureDirectoryExists(const std::wstring& path)
{
	DWORD attr = GetFileAttributesW(path.c_str());
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return true;

	if (CreateDirectoryW(path.c_str(), NULL))
		return true;

	// If parent doesn't exist, create it first
	if (GetLastError() == ERROR_PATH_NOT_FOUND)
	{
		size_t pos = path.find_last_of(L'\\', path.length() - 2);
		if (pos != std::wstring::npos)
		{
			EnsureDirectoryExists(path.substr(0, pos + 1));
			return CreateDirectoryW(path.c_str(), NULL) != FALSE;
		}
	}
	return false;
}

bool Win64SavePaths::Initialize()
{
	if (s_initialized)
		return true;

	// Get %APPDATA% via SHGetFolderPathW
	WCHAR appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		OutputDebugStringA("Win64SavePaths: Failed to get APPDATA path\n");
		return false;
	}

	// Build base directory: %APPDATA%\Minecraft Console Edition\
	s_baseDir = appDataPath;
	if (s_baseDir.back() != L'\\')
		s_baseDir += L'\\';
	s_baseDir += L"Minecraft Console Edition\\";

	// Build subdirectory paths
	s_settingsDir    = s_baseDir + L"settings\\";
	s_worldSavesDir  = s_baseDir + L"saves\\";
	s_profilesDir    = s_baseDir + L"profiles\\";
	s_dlcDir         = s_baseDir + L"dlc\\";
	s_screenshotsDir = s_baseDir + L"screenshots\\";

	// Create all directories
	bool ok = true;
	ok = EnsureDirectoryExists(s_baseDir)        && ok;
	ok = EnsureDirectoryExists(s_settingsDir)    && ok;
	ok = EnsureDirectoryExists(s_worldSavesDir)  && ok;
	ok = EnsureDirectoryExists(s_profilesDir)    && ok;
	ok = EnsureDirectoryExists(s_dlcDir)         && ok;
	ok = EnsureDirectoryExists(s_screenshotsDir) && ok;

	if (!ok)
	{
		OutputDebugStringA("Win64SavePaths: Failed to create one or more directories\n");
	}

	s_initialized = true;
	return ok;
}

const std::wstring& Win64SavePaths::GetBaseDir()        { return s_baseDir; }
const std::wstring& Win64SavePaths::GetSettingsDir()    { return s_settingsDir; }
const std::wstring& Win64SavePaths::GetWorldSavesDir()  { return s_worldSavesDir; }
const std::wstring& Win64SavePaths::GetProfilesDir()    { return s_profilesDir; }
const std::wstring& Win64SavePaths::GetDLCDir()         { return s_dlcDir; }
const std::wstring& Win64SavePaths::GetScreenshotsDir() { return s_screenshotsDir; }
