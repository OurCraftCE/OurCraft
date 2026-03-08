#pragma once
#include <string>
#include <windows.h>

// Static class to manage PC save directory paths
// All paths are under %APPDATA%/Minecraft Console Edition/
class Win64SavePaths
{
public:
	// Must be called once at startup before any file I/O
	static bool Initialize();

	// Directory getters (all return paths with trailing backslash)
	static const std::wstring& GetBaseDir();       // %APPDATA%/Minecraft Console Edition/
	static const std::wstring& GetSettingsDir();    // .../settings/
	static const std::wstring& GetWorldSavesDir();  // .../saves/
	static const std::wstring& GetProfilesDir();    // .../profiles/
	static const std::wstring& GetDLCDir();         // .../dlc/
	static const std::wstring& GetScreenshotsDir(); // .../screenshots/

	static bool IsInitialized() { return s_initialized; }

private:
	static bool s_initialized;
	static std::wstring s_baseDir;
	static std::wstring s_settingsDir;
	static std::wstring s_worldSavesDir;
	static std::wstring s_profilesDir;
	static std::wstring s_dlcDir;
	static std::wstring s_screenshotsDir;

	// Ensures a directory exists, creating it if needed
	static bool EnsureDirectoryExists(const std::wstring& path);
};
