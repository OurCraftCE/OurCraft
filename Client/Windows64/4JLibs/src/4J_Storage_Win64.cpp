#include "stdafx.h"
#include "..\..\..\Platform\StorageManager.h"
#include "..\..\Win64SavePaths.h"
#include <shlobj.h>
#include <io.h>

// Global storage manager instance
CStorageManager StorageManager;

// Internal state for the PC implementation
static bool s_saveDisabled = false;
static bool s_saveDeviceSelected[XUSER_MAX_COUNT] = {};
static SAVE_DETAILS s_saveDetails = {};
static std::vector<SAVE_INFO> s_saveInfoList;
static std::vector<BYTE> s_saveBuffer;
static BYTE* s_allocatedSaveData = NULL;
static unsigned int s_allocatedSaveSize = 0;
static unsigned int s_saveVersion = 0;
static std::wstring s_defaultSaveName;
static std::wstring s_saveTitle;
static std::string s_savePackName;
static std::wstring s_saveFilename;
static PBYTE s_thumbnailData = NULL;
static DWORD s_thumbnailBytes = 0;

// Forward declarations for internal helpers
static std::wstring GetSaveBasePath();
static std::wstring GetSaveFolderPath(const char* saveName);
static bool WriteFileAtomic(const std::wstring& path, const void* data, DWORD size);

//----------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------
static std::wstring GetSaveBasePath()
{
	return Win64SavePaths::GetWorldSavesDir();
}

static std::wstring GetSaveFolderPath(const char* saveName)
{
	std::wstring base = GetSaveBasePath();
	std::wstring wideName;
	for (const char* p = saveName; *p; ++p)
		wideName += (wchar_t)*p;
	return base + wideName + L"\\";
}

static bool WriteFileAtomic(const std::wstring& path, const void* data, DWORD size)
{
	std::wstring tmpPath = path + L".tmp";

	HANDLE hFile = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD written = 0;
	BOOL ok = WriteFile(hFile, data, size, &written, NULL);
	FlushFileBuffers(hFile);
	CloseHandle(hFile);

	if (!ok || written != size)
	{
		DeleteFileW(tmpPath.c_str());
		return false;
	}

	// Atomic replace
	DeleteFileW(path.c_str());
	if (!MoveFileW(tmpPath.c_str(), path.c_str()))
	{
		CopyFileW(tmpPath.c_str(), path.c_str(), FALSE);
		DeleteFileW(tmpPath.c_str());
	}

	return true;
}

//----------------------------------------------------------------------
// CStorageManager implementation
//----------------------------------------------------------------------

CStorageManager::CStorageManager()
{
	m_pStringTable = NULL;
}

void CStorageManager::Tick(void)
{
	// No async operations to tick on PC
}

// Messages - minimal stubs for PC
CStorageManager::EMessageResult CStorageManager::RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA, UINT uiOptionC, DWORD dwPad,
	int(*Func)(LPVOID, int, const CStorageManager::EMessageResult), LPVOID lpParam, C4JStringTable *pStringTable, WCHAR *pwchFormatString, DWORD dwFocusButton)
{
	// On PC, immediately return accept
	return EMessage_ResultAccept;
}

CStorageManager::EMessageResult CStorageManager::GetMessageBoxResult()
{
	return EMessage_ResultAccept;
}

// Save device - on PC, always selected
bool CStorageManager::SetSaveDevice(int(*Func)(LPVOID, const bool), LPVOID lpParam, bool bForceResetOfSaveDevice)
{
	if (Func) Func(lpParam, true);
	return true;
}

void CStorageManager::Init(unsigned int uiSaveVersion, LPCWSTR pwchDefaultSaveName, char *pszSavePackName,
	int iMinimumSaveSize, int(*Func)(LPVOID, const ESavingMessage, int), LPVOID lpParam, LPCSTR szGroupID)
{
	s_saveVersion = uiSaveVersion;
	if (pwchDefaultSaveName) s_defaultSaveName = pwchDefaultSaveName;
	if (pszSavePackName) s_savePackName = pszSavePackName;
}

void CStorageManager::ResetSaveData()
{
	s_saveFilename.clear();
	if (s_allocatedSaveData)
	{
		delete[] s_allocatedSaveData;
		s_allocatedSaveData = NULL;
	}
	s_allocatedSaveSize = 0;
}

void CStorageManager::SetDefaultSaveNameForKeyboardDisplay(LPCWSTR pwchDefaultSaveName)
{
	if (pwchDefaultSaveName) s_defaultSaveName = pwchDefaultSaveName;
}

void CStorageManager::SetSaveTitle(LPCWSTR pwchDefaultSaveName)
{
	if (pwchDefaultSaveName) s_saveTitle = pwchDefaultSaveName;
}

bool CStorageManager::GetSaveUniqueNumber(INT *piVal)
{
	// Find next available save number by scanning directory
	std::wstring basePath = GetSaveBasePath();
	int maxNum = 0;

	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW((basePath + L"*").c_str(), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Try to extract a number from directory name
				std::wstring name = findData.cFileName;
				if (name != L"." && name != L"..")
					maxNum++;
			}
		} while (FindNextFileW(hFind, &findData));
		FindClose(hFind);
	}

	*piVal = maxNum + 1;
	return true;
}

bool CStorageManager::GetSaveUniqueFilename(char *pszName)
{
	INT num = 0;
	GetSaveUniqueNumber(&num);
	sprintf_s(pszName, MAX_SAVEFILENAME_LENGTH, "save_%d", num);
	return true;
}

void CStorageManager::SetSaveUniqueFilename(char *szFilename)
{
	if (szFilename)
	{
		s_saveFilename.clear();
		for (const char* p = szFilename; *p; ++p)
			s_saveFilename += (wchar_t)*p;
	}
}

void CStorageManager::SetState(ESaveGameControlState eControlState, int(*Func)(LPVOID, const bool), LPVOID lpParam)
{
	if (Func) Func(lpParam, true);
}

void CStorageManager::SetSaveDisabled(bool bDisable)
{
	s_saveDisabled = bDisable;
}

bool CStorageManager::GetSaveDisabled(void)
{
	return s_saveDisabled;
}

unsigned int CStorageManager::GetSaveSize()
{
	return s_allocatedSaveSize;
}

void CStorageManager::GetSaveData(void *pvData, unsigned int *puiBytes)
{
	if (s_allocatedSaveData && s_allocatedSaveSize > 0)
	{
		memcpy(pvData, s_allocatedSaveData, s_allocatedSaveSize);
		*puiBytes = s_allocatedSaveSize;
	}
	else
	{
		*puiBytes = 0;
	}
}

PVOID CStorageManager::AllocateSaveData(unsigned int uiBytes)
{
	if (s_allocatedSaveData) delete[] s_allocatedSaveData;
	s_allocatedSaveData = new BYTE[uiBytes];
	s_allocatedSaveSize = uiBytes;
	ZeroMemory(s_allocatedSaveData, uiBytes);
	return s_allocatedSaveData;
}

void CStorageManager::SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes)
{
	s_thumbnailData = pbThumbnail;
	s_thumbnailBytes = dwThumbnailBytes;
}

CStorageManager::ESaveGameState CStorageManager::SaveSaveData(int(*Func)(LPVOID, const bool), LPVOID lpParam)
{
	bool success = false;

	if (s_allocatedSaveData && s_allocatedSaveSize > 0 && !s_saveFilename.empty())
	{
		std::wstring savePath = GetSaveBasePath() + s_saveFilename + L"\\";
		CreateDirectoryW(savePath.c_str(), NULL);

		std::wstring dataPath = savePath + L"savedata.bin";
		success = WriteFileAtomic(dataPath, s_allocatedSaveData, s_allocatedSaveSize);

		// Save thumbnail if available
		if (s_thumbnailData && s_thumbnailBytes > 0)
		{
			std::wstring thumbPath = savePath + L"thumbnail.png";
			WriteFileAtomic(thumbPath, s_thumbnailData, s_thumbnailBytes);
		}

		// Save world name metadata for display in load screen
		if (!s_saveTitle.empty())
		{
			std::wstring metaPath = savePath + L"worldname.txt";
			WriteFileAtomic(metaPath, s_saveTitle.c_str(), (DWORD)(s_saveTitle.size() * sizeof(wchar_t)));
		}
	}

	if (Func) Func(lpParam, success);
	return ESaveGame_Idle;
}

void CStorageManager::CopySaveDataToNewSave(PBYTE pbThumbnail, DWORD cbThumbnail, WCHAR *wchNewName, int(*Func)(LPVOID lpParam, bool), LPVOID lpParam)
{
	if (Func) Func(lpParam, true);
}

void CStorageManager::SetSaveDeviceSelected(unsigned int uiPad, bool bSelected)
{
	if (uiPad < XUSER_MAX_COUNT)
		s_saveDeviceSelected[uiPad] = bSelected;
}

bool CStorageManager::GetSaveDeviceSelected(unsigned int iPad)
{
	if (iPad < XUSER_MAX_COUNT)
		return s_saveDeviceSelected[iPad];
	return true; // On PC, always available
}

CStorageManager::ESaveGameState CStorageManager::DoesSaveExist(bool *pbExists)
{
	if (!s_saveFilename.empty())
	{
		std::wstring savePath = GetSaveBasePath() + s_saveFilename + L"\\savedata.bin";
		DWORD attr = GetFileAttributesW(savePath.c_str());
		*pbExists = (attr != INVALID_FILE_ATTRIBUTES);
	}
	else
	{
		*pbExists = false;
	}
	return ESaveGame_Idle;
}

bool CStorageManager::EnoughSpaceForAMinSaveGame()
{
	// On PC, assume enough space unless disk is truly full
	ULARGE_INTEGER freeBytes;
	std::wstring basePath = GetSaveBasePath();
	if (GetDiskFreeSpaceExW(basePath.c_str(), &freeBytes, NULL, NULL))
	{
		return freeBytes.QuadPart > (10 * 1024 * 1024); // need at least 10MB
	}
	return true;
}

void CStorageManager::SetSaveMessageVPosition(float fY)
{
	// No save message overlay on PC
}

// Enumerate saves using FindFirstFile/FindNextFile
CStorageManager::ESaveGameState CStorageManager::GetSavesInfo(int iPad, int(*Func)(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, const bool), LPVOID lpParam, char *pszSavePackName)
{
	s_saveInfoList.clear();

	std::wstring basePath = GetSaveBasePath();
	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW((basePath + L"*").c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
				continue;

			// Check if this directory contains save data
			std::wstring saveDataPath = basePath + findData.cFileName + L"\\savedata.bin";
			DWORD attr = GetFileAttributesW(saveDataPath.c_str());
			if (attr == INVALID_FILE_ATTRIBUTES)
			{
				// Also check for level.dat (McRegion format)
				saveDataPath = basePath + findData.cFileName + L"\\level.dat";
				attr = GetFileAttributesW(saveDataPath.c_str());
				if (attr == INVALID_FILE_ATTRIBUTES)
					continue;
			}

			SAVE_INFO info;
			ZeroMemory(&info, sizeof(info));

			// Convert wide filename to UTF8
			char utf8Name[MAX_SAVEFILENAME_LENGTH];
			WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, utf8Name, MAX_SAVEFILENAME_LENGTH, NULL, NULL);
			strncpy_s(info.UTF8SaveFilename, utf8Name, MAX_SAVEFILENAME_LENGTH - 1);

			// Try to read world name from worldname.txt metadata
			std::wstring worldNamePath = basePath + findData.cFileName + L"\\worldname.txt";
			std::wstring displayName;
			HANDLE hMeta = CreateFileW(worldNamePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hMeta != INVALID_HANDLE_VALUE)
			{
				DWORD metaSize = GetFileSize(hMeta, NULL);
				if (metaSize > 0 && metaSize < 1024)
				{
					wchar_t metaBuf[512];
					DWORD bytesRead = 0;
					if (ReadFile(hMeta, metaBuf, metaSize, &bytesRead, NULL))
					{
						int charCount = bytesRead / sizeof(wchar_t);
						displayName.assign(metaBuf, charCount);
					}
				}
				CloseHandle(hMeta);
			}

			// Fall back to folder name if no metadata
			if (displayName.empty())
				displayName = findData.cFileName;

			char utf8Title[MAX_DISPLAYNAME_LENGTH];
			WideCharToMultiByte(CP_UTF8, 0, displayName.c_str(), -1, utf8Title, MAX_DISPLAYNAME_LENGTH, NULL, NULL);
			strncpy_s(info.UTF8SaveTitle, utf8Title, MAX_DISPLAYNAME_LENGTH - 1);

			// Get file time
			FILETIME ft = findData.ftLastWriteTime;
			ULARGE_INTEGER ull;
			ull.LowPart = ft.dwLowDateTime;
			ull.HighPart = ft.dwHighDateTime;
			info.metaData.modifiedTime = (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);

			// Get data size
			HANDLE hData = CreateFileW(saveDataPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hData != INVALID_HANDLE_VALUE)
			{
				info.metaData.dataSize = GetFileSize(hData, NULL);
				CloseHandle(hData);
			}

			info.thumbnailData = NULL;
			info.metaData.thumbnailSize = 0;
			info.needsSync = false;

			s_saveInfoList.push_back(info);

		} while (FindNextFileW(hFind, &findData));
		FindClose(hFind);
	}

	// Build SAVE_DETAILS
	s_saveDetails.iSaveC = (int)s_saveInfoList.size();
	s_saveDetails.SaveInfoA = s_saveInfoList.empty() ? NULL : &s_saveInfoList[0];

	if (Func) Func(lpParam, &s_saveDetails, true);
	return ESaveGame_GetSavesInfo;
}

PSAVE_DETAILS CStorageManager::ReturnSavesInfo()
{
	return &s_saveDetails;
}

void CStorageManager::ClearSavesInfo()
{
	s_saveInfoList.clear();
	s_saveDetails.iSaveC = 0;
	s_saveDetails.SaveInfoA = NULL;
}

CStorageManager::ESaveGameState CStorageManager::LoadSaveDataThumbnail(PSAVE_INFO pSaveInfo, int(*Func)(LPVOID lpParam, PBYTE pbThumbnail, DWORD dwThumbnailBytes), LPVOID lpParam, bool bForceLoad)
{
	if (Func) Func(lpParam, NULL, 0);
	return ESaveGame_GetSaveThumbnail;
}

CStorageManager::ESaveGameState CStorageManager::RenameSaveData(int iSaveIndex, uint16_t *pwchNewName, int(*Func)(LPVOID, bool), LPVOID lpParam)
{
	if (Func) Func(lpParam, true);
	return ESaveGame_Rename;
}

CStorageManager::ESaveGameState CStorageManager::GetSaveState()
{
	return ESaveGame_Idle;
}

void CStorageManager::ReadFromProfile(int iQuadrant)
{
	// No-op on PC
}

void CStorageManager::GetSaveCacheFileInfo(DWORD dwFile, XCONTENT_DATA &xContentData)
{
}

void CStorageManager::GetSaveCacheFileInfo(DWORD dwFile, PBYTE *ppbImageData, DWORD *pdwImageBytes)
{
	if (ppbImageData) *ppbImageData = NULL;
	if (pdwImageBytes) *pdwImageBytes = 0;
}

CStorageManager::ESaveGameState CStorageManager::LoadSaveData(PSAVE_INFO pSaveInfo, int(*Func)(LPVOID lpParam, const bool, const bool), LPVOID lpParam)
{
	bool success = false;

	if (pSaveInfo)
	{
		std::wstring wideName;
		for (const char* p = pSaveInfo->UTF8SaveFilename; *p; ++p)
			wideName += (wchar_t)*p;

		std::wstring dataPath = GetSaveBasePath() + wideName + L"\\savedata.bin";
		HANDLE hFile = CreateFileW(dataPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD fileSize = GetFileSize(hFile, NULL);
			if (fileSize > 0)
			{
				AllocateSaveData(fileSize);
				DWORD bytesRead = 0;
				if (ReadFile(hFile, s_allocatedSaveData, fileSize, &bytesRead, NULL) && bytesRead == fileSize)
				{
					success = true;
				}
			}
			CloseHandle(hFile);
		}
	}

	if (Func) Func(lpParam, success, false);
	return ESaveGame_Load;
}

CStorageManager::ESaveGameState CStorageManager::DeleteSaveData(PSAVE_INFO pSaveInfo, int(*Func)(LPVOID lpParam, const bool), LPVOID lpParam)
{
	bool success = false;

	if (pSaveInfo)
	{
		std::wstring wideName;
		for (const char* p = pSaveInfo->UTF8SaveFilename; *p; ++p)
			wideName += (wchar_t)*p;

		std::wstring savePath = GetSaveBasePath() + wideName;

		// Delete all files in directory then the directory itself
		WIN32_FIND_DATAW fd;
		HANDLE hFind = FindFirstFileW((savePath + L"\\*").c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
				{
					DeleteFileW((savePath + L"\\" + fd.cFileName).c_str());
				}
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
		success = RemoveDirectoryW(savePath.c_str()) != FALSE;
	}

	if (Func) Func(lpParam, success);
	return ESaveGame_Delete;
}

// DLC stubs - not needed for PC filesystem saves
void CStorageManager::RegisterMarketplaceCountsCallback(int(*Func)(LPVOID lpParam, CStorageManager::DLC_TMS_DETAILS *, int), LPVOID lpParam) {}
void CStorageManager::SetDLCPackageRoot(char *pszDLCRoot) {}
CStorageManager::EDLCStatus CStorageManager::GetDLCOffers(int iPad, int(*Func)(LPVOID, int, DWORD, int), LPVOID lpParam, DWORD dwOfferTypesBitmask) { return EDLC_NoOffers; }
DWORD CStorageManager::CancelGetDLCOffers() { return 0; }
void CStorageManager::ClearDLCOffers() {}
XMARKETPLACE_CONTENTOFFER_INFO& CStorageManager::GetOffer(DWORD dw) { static XMARKETPLACE_CONTENTOFFER_INFO dummy = {}; return dummy; }
int CStorageManager::GetOfferCount() { return 0; }
DWORD CStorageManager::InstallOffer(int iOfferIDC, __uint64 *ullOfferIDA, int(*Func)(LPVOID, int, int), LPVOID lpParam, bool bTrial) { return 0; }
DWORD CStorageManager::InstallOffer(int iOfferIDC, LPCWSTR wszProductId, int(*Func)(LPVOID, int, int), LPVOID lpParam, bool bTrial) { return 0; }
DWORD CStorageManager::GetAvailableDLCCount(int iPad) { return 0; }
CStorageManager::EDLCStatus CStorageManager::GetInstalledDLC(int iPad, int(*Func)(LPVOID, int, int), LPVOID lpParam) { return EDLC_NoInstalledDLC; }
XCONTENT_DATA& CStorageManager::GetDLC(DWORD dw) { static XCONTENT_DATA dummy = {}; return dummy; }
DWORD CStorageManager::MountInstalledDLC(int iPad, DWORD dwDLC, int(*Func)(LPVOID, int, DWORD, DWORD), LPVOID lpParam, LPCSTR szMountDrive) { return 0; }
DWORD CStorageManager::UnmountInstalledDLC(LPCSTR szMountDrive) { return 0; }
DWORD CStorageManager::UnmountInstalledDLC(LPCWSTR wszMountDrive) { return 0; }
void CStorageManager::GetMountedDLCFileList(const char* szMountDrive, std::vector<std::string>& fileList) {}
std::string CStorageManager::GetMountedPath(std::string szMount) { return szMount; }
std::wstring CStorageManager::GetMountedPath(std::wstring wszMount) { return wszMount; }
XCONTENT_DATA* CStorageManager::GetInstalledDLC(WCHAR *wszProductID) { return NULL; }

// TMS stubs
CStorageManager::ETMSStatus CStorageManager::ReadTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, CStorageManager::eTMS_FileType eFileType,
	WCHAR *pwchFilename, BYTE **ppBuffer, DWORD *pdwBufferSize, int(*Func)(LPVOID, WCHAR *, int, bool, int), LPVOID lpParam, int iAction)
{
	return ETMSStatus_Idle;
}

bool CStorageManager::WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, WCHAR *pwchFilename, BYTE *pBuffer, DWORD dwBufferSize) { return false; }
bool CStorageManager::DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, WCHAR *pwchFilename) { return false; }
void CStorageManager::StoreTMSPathName(WCHAR *pwchName) {}

// TMS++ stubs
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFile(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, LPCWSTR pwchFilePath, PBYTE pBuffer, DWORD dwBufferSize, int(*Func)(LPVOID, int, int), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFile(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, eTMS_UGCTYPE eUGCType, LPCSTR pchFilePath, PCHAR pchBuffer, DWORD dwBufferSize, int(*Func)(LPVOID, int, int), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_WriteFileWithProgress(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, eTMS_UGCTYPE eUGCType, LPCSTR pchFilePath, PCHAR pchBuffer, DWORD dwBufferSize, int(*Func)(LPVOID, int, int), LPVOID lpParam, int iUserData, int(*ProgressFunc)(LPVOID, float), LPVOID lpProgressParam) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFile(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, LPCSTR szFilename, int(*Func)(LPVOID, int, int, PTMSPP_FILEDATA, LPCSTR), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFile(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, LPCWSTR wszFilename, int(*Func)(LPVOID, int, int, LPVOID, WCHAR*), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_ReadFileList(int iPad, eGlobalStorage eStorageFacility, int(*Func)(LPVOID, int, int, LPVOID, WCHAR*), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_DeleteFile(int iPad, LPCSTR szFilePath, eTMS_FILETYPEVAL eFileTypeVal, int(*Func)(LPVOID, int, int), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
CStorageManager::ETMSStatus CStorageManager::TMSPP_DeleteFile(int iPad, eGlobalStorage eStorageFacility, eTMS_FILETYPEVAL eFileTypeVal, LPCWSTR pwchFilePath, int(*Func)(LPVOID, int, int), LPVOID lpParam, int iUserData) { return ETMSStatus_Idle; }
bool CStorageManager::TMSPP_InFileList(eGlobalStorage eStorageFacility, int iPad, const wstring &Filename) { return false; }

// Save transfer stubs
CStorageManager::eSaveTransferState CStorageManager::SaveTransferGetDetails(int iPad, eGlobalStorage eStorageFacility, WCHAR *pwchFilename, int(*Func)(LPVOID, SAVETRANSFER_FILE_DETAILS *), LPVOID lpParam) { return eSaveTransfer_Idle; }
CStorageManager::eSaveTransferState CStorageManager::SaveTransferGetData(int iPad, eGlobalStorage eStorageFacility, WCHAR *pwchFilename, int(*Func)(LPVOID, SAVETRANSFER_FILE_DETAILS *), int(*ProgressFunc)(LPVOID, unsigned long), LPVOID lpParam, LPVOID lpProgressParam) { return eSaveTransfer_Idle; }
CStorageManager::eSaveTransferState CStorageManager::SaveTransferClearState() { return eSaveTransfer_Idle; }
void CStorageManager::CancelSaveTransfer(int(*Func)(LPVOID), LPVOID lpParam) { if (Func) Func(lpParam); }

unsigned int CStorageManager::CRC(unsigned char *buf, int len)
{
	unsigned int crc = 0xFFFFFFFF;
	for (int i = 0; i < len; i++)
	{
		crc ^= buf[i];
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

void CStorageManager::ContinueIncompleteOperation()
{
}

// Options callbacks - PC uses file-based options
CStorageManager::eOptionsCallback s_optionsCallbackStatus[XUSER_MAX_COUNT] = {};
