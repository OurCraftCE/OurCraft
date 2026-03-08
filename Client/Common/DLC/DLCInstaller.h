#pragma once

#include <string>
using namespace std;

class DLCPack;
class DLCManager;
class DLCCatalog;
struct DLCCatalogEntry;

class DLCInstaller
{
public:
	enum EInstallStatus
	{
		eStatus_NotInstalled = 0,
		eStatus_Installing,
		eStatus_Installed,
		eStatus_Error,
	};

	DLCInstaller();
	~DLCInstaller();

	// Set the base paths for available and installed DLC
	void SetPaths(const wstring &availablePath, const wstring &installedPath);

	// Install a DLC pack from the available directory to the installed directory
	// Copies files from availablePath/<packId> to installedPath/<packId>
	EInstallStatus InstallFromLocal(const wstring &packId);

	// Uninstall a DLC pack by removing its installed directory
	bool Uninstall(const wstring &packId);

	// Load an installed pack into the DLCManager
	// Returns the DLCPack pointer, or NULL on failure
	DLCPack *LoadInstalledPack(const wstring &packId, DLCManager &dlcManager);

	// Get the install status of a specific pack
	EInstallStatus GetStatus(const wstring &packId) const;

	// Scan the installed directory and load all packs into DLCManager
	int LoadAllInstalled(DLCManager &dlcManager);

private:
	wstring m_availablePath;   // e.g. "dlc\\available"
	wstring m_installedPath;   // e.g. "dlc\\installed"

	// Recursively copy directory contents
	bool CopyDirectoryContents(const wstring &srcDir, const wstring &dstDir);

	// Recursively delete a directory
	bool DeleteDirectoryRecursive(const wstring &dirPath);

	// Create directory tree
	bool EnsureDirectoryExists(const wstring &path);
};
