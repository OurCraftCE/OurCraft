#include "stdafx.h"
#include "DLCInstaller.h"
#include "DLCManager.h"
#include "DLCPack.h"

DLCInstaller::DLCInstaller()
{
}

DLCInstaller::~DLCInstaller()
{
}

void DLCInstaller::SetPaths(const wstring &availablePath, const wstring &installedPath)
{
	m_availablePath = availablePath;
	m_installedPath = installedPath;
}

bool DLCInstaller::EnsureDirectoryExists(const wstring &path)
{
	DWORD attrib = GetFileAttributesW(path.c_str());
	if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return true;

	// Try to create parent first
	size_t pos = path.find_last_of(L"\\/");
	if (pos != wstring::npos && pos > 0)
	{
		wstring parent = path.substr(0, pos);
		EnsureDirectoryExists(parent);
	}

	return CreateDirectoryW(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool DLCInstaller::CopyDirectoryContents(const wstring &srcDir, const wstring &dstDir)
{
	if (!EnsureDirectoryExists(dstDir))
		return false;

	WIN32_FIND_DATAW wfd;
	wstring searchPath = srcDir + L"\\*";
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &wfd);

	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	bool success = true;
	do
	{
		wstring fileName = wfd.cFileName;
		if (fileName == L"." || fileName == L"..")
			continue;

		wstring srcPath = srcDir + L"\\" + fileName;
		wstring dstPath = dstDir + L"\\" + fileName;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!CopyDirectoryContents(srcPath, dstPath))
				success = false;
		}
		else
		{
			if (!CopyFileW(srcPath.c_str(), dstPath.c_str(), FALSE))
			{
				app.DebugPrintf("DLCInstaller: Failed to copy file %ls -> %ls\n", srcPath.c_str(), dstPath.c_str());
				success = false;
			}
		}
	} while (FindNextFileW(hFind, &wfd));

	FindClose(hFind);
	return success;
}

bool DLCInstaller::DeleteDirectoryRecursive(const wstring &dirPath)
{
	WIN32_FIND_DATAW wfd;
	wstring searchPath = dirPath + L"\\*";
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &wfd);

	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		wstring fileName = wfd.cFileName;
		if (fileName == L"." || fileName == L"..")
			continue;

		wstring fullPath = dirPath + L"\\" + fileName;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DeleteDirectoryRecursive(fullPath);
		}
		else
		{
			DeleteFileW(fullPath.c_str());
		}
	} while (FindNextFileW(hFind, &wfd));

	FindClose(hFind);
	return RemoveDirectoryW(dirPath.c_str()) != 0;
}

DLCInstaller::EInstallStatus DLCInstaller::InstallFromLocal(const wstring &packId)
{
	wstring srcDir = m_availablePath + L"\\" + packId;
	wstring dstDir = m_installedPath + L"\\" + packId;

	// Check source exists
	DWORD srcAttrib = GetFileAttributesW(srcDir.c_str());
	if (srcAttrib == INVALID_FILE_ATTRIBUTES || !(srcAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		app.DebugPrintf("DLCInstaller: Source directory not found for pack %ls\n", packId.c_str());
		return eStatus_Error;
	}

	// If already installed, remove old version first
	DWORD dstAttrib = GetFileAttributesW(dstDir.c_str());
	if (dstAttrib != INVALID_FILE_ATTRIBUTES && (dstAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		DeleteDirectoryRecursive(dstDir);
	}

	app.DebugPrintf("DLCInstaller: Installing pack %ls from %ls\n", packId.c_str(), srcDir.c_str());

	if (!CopyDirectoryContents(srcDir, dstDir))
	{
		app.DebugPrintf("DLCInstaller: Failed to install pack %ls\n", packId.c_str());
		return eStatus_Error;
	}

	app.DebugPrintf("DLCInstaller: Pack %ls installed successfully\n", packId.c_str());
	return eStatus_Installed;
}

bool DLCInstaller::Uninstall(const wstring &packId)
{
	wstring dstDir = m_installedPath + L"\\" + packId;

	DWORD attrib = GetFileAttributesW(dstDir.c_str());
	if (attrib == INVALID_FILE_ATTRIBUTES)
	{
		app.DebugPrintf("DLCInstaller: Pack %ls is not installed\n", packId.c_str());
		return false;
	}

	app.DebugPrintf("DLCInstaller: Uninstalling pack %ls\n", packId.c_str());
	return DeleteDirectoryRecursive(dstDir);
}

DLCPack *DLCInstaller::LoadInstalledPack(const wstring &packId, DLCManager &dlcManager)
{
	wstring packDir = m_installedPath + L"\\" + packId;

	DWORD attrib = GetFileAttributesW(packDir.c_str());
	if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		app.DebugPrintf("DLCInstaller: Installed pack directory not found for %ls\n", packId.c_str());
		return NULL;
	}

	// Check if pack already loaded
	DLCPack *existing = dlcManager.getPack(packId);
	if (existing != NULL)
	{
		app.DebugPrintf("DLCInstaller: Pack %ls already loaded\n", packId.c_str());
		return existing;
	}

	// Create a new DLCPack - treat all local installs as fully licensed (0xFFFFFFFF)
	DLCPack *pack = new DLCPack(packId, packId, 0xFFFFFFFF);
	dlcManager.addPack(pack);

	// Enumerate files in the pack directory and read DLC data files
	WIN32_FIND_DATAW wfd;
	WCHAR szSearchPath[MAX_PATH];
	swprintf(szSearchPath, MAX_PATH, L"%ls\\*", packDir.c_str());
	HANDLE hFind = FindFirstFileW(szSearchPath, &wfd);

	DWORD dwFilesProcessed = 0;

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				WCHAR szFullPath[MAX_PATH];
				swprintf(szFullPath, MAX_PATH, L"%ls\\%ls", packDir.c_str(), wfd.cFileName);
				dlcManager.readDLCDataFile(dwFilesProcessed, wstring(szFullPath), pack);
			}
		} while (FindNextFileW(hFind, &wfd));

		FindClose(hFind);
	}

	if (dwFilesProcessed == 0)
	{
		app.DebugPrintf("DLCInstaller: No data files in pack %ls, removing\n", packId.c_str());
		dlcManager.removePack(pack);
		return NULL;
	}

	app.DebugPrintf("DLCInstaller: Loaded pack %ls (%d files)\n", packId.c_str(), dwFilesProcessed);
	return pack;
}

DLCInstaller::EInstallStatus DLCInstaller::GetStatus(const wstring &packId) const
{
	wstring packDir = m_installedPath + L"\\" + packId;
	DWORD attrib = GetFileAttributesW(packDir.c_str());
	if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return eStatus_Installed;
	return eStatus_NotInstalled;
}

int DLCInstaller::LoadAllInstalled(DLCManager &dlcManager)
{
	int loadedCount = 0;

	WIN32_FIND_DATAW wfd;
	wstring searchPath = m_installedPath + L"\\*";
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &wfd);

	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do
	{
		wstring dirName = wfd.cFileName;
		if (dirName == L"." || dirName == L"..")
			continue;

		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DLCPack *pack = LoadInstalledPack(dirName, dlcManager);
			if (pack != NULL)
				loadedCount++;
		}
	} while (FindNextFileW(hFind, &wfd));

	FindClose(hFind);

	app.DebugPrintf("DLCInstaller: Loaded %d installed packs from %ls\n", loadedCount, m_installedPath.c_str());
	return loadedCount;
}
