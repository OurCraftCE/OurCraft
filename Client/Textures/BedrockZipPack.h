#pragma once
#include "AbstractTexturePack.h"
#include "BedrockResourcePack.h"

class BedrockZipPack : public AbstractTexturePack
{
public:
	BedrockZipPack(DWORD id, File *zipFile, TexturePack *fallback);
	~BedrockZipPack();

	void unload(Textures *textures);
	bool hasFile(const wstring &name);
	bool hasData() { return m_bedrockPack.isLoaded(); }
	bool isLoadingData() { return false; }
	bool isTerrainUpdateCompatible() { return true; }
	DLCPack *getDLCPack() { return NULL; }
	BedrockResourcePack *getBedrockPack() { return &m_bedrockPack; }

protected:
	InputStream *getResourceImplementation(const wstring &name);

private:
	void openZip();
	void closeZip();
	byteArray extractFile(const string &entryName);

	void *m_zipHandle;
	bool m_zipOpen;
	wstring m_zipPath;
	BedrockResourcePack m_bedrockPack;
	wstring m_tempDir;
};
