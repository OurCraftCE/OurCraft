#pragma once

#include "AbstractTexturePack.h"
#include "BedrockResourcePack.h"

class BedrockFolderPack : public AbstractTexturePack
{
public:
	BedrockFolderPack(DWORD id, const wstring &name, File *folder, TexturePack *fallback);

	void unload(Textures *textures);
	bool hasFile(const wstring &name);
	bool hasData() { return m_bedrockPack.isLoaded(); }
	bool isLoadingData() { return false; }
	bool isTerrainUpdateCompatible() { return true; }
	DLCPack *getDLCPack() { return NULL; }
	BedrockResourcePack *getBedrockPack() { return &m_bedrockPack; }
	wstring getPath(bool bTitleUpdateTexture = false);

protected:
	InputStream *getResourceImplementation(const wstring &name);

private:
	BedrockResourcePack m_bedrockPack;
};
