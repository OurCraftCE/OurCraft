#pragma once

class CConsoleMinecraftApp : public CMinecraftApp
{
public:
	CConsoleMinecraftApp();

	virtual void SetRichPresenceContext(int iPad, int contextId);

	virtual void StoreLaunchData();
	virtual void ExitGame();
	virtual void FatalLoadError();

	virtual void CaptureSaveThumbnail();
	virtual void GetSaveThumbnail(PBYTE*,DWORD*);
	virtual void ReleaseSaveThumbnail();
	virtual void GetScreenshot(int iPad,PBYTE *pbData,DWORD *pdwSize);

	virtual int LoadLocalTMSFile(WCHAR *wchTMSFile);
	virtual int LoadLocalTMSFile(WCHAR *wchTMSFile, eFileExtensionType eExt);

	int LoadLocalDLCImages();
	void FreeLocalDLCImages();
	void TMSPP_RetrieveFileList(int iPad,CStorageManager::eGlobalStorage eStorageFacility,eTMSAction NextAction);

	virtual void FreeLocalTMSFiles(eTMSFileType eType);
	virtual int GetLocalTMSFileIndex(WCHAR *wchTMSFile,bool bFilenameIncludesExtension,eFileExtensionType eEXT=eFileExtensionType_PNG);

	// BANNED LEVEL LIST
	virtual void ReadBannedList(int iPad, eTMSAction action=(eTMSAction)0, bool bCallback=false) {}
	bool TMSPP_ReadBannedList(int iPad, eTMSAction NextAction) { return false; }

	C4JStringTable *GetStringTable()																									{ return NULL;}

	// original code
	virtual void TemporaryCreateGameStart();
};

extern CConsoleMinecraftApp app;
