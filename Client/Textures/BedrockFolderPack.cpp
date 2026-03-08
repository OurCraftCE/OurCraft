#include "stdafx.h"
#include "BedrockFolderPack.h"
#include "../World/IO/File.h"
#include "../World/IO/InputStream.h"

BedrockFolderPack::BedrockFolderPack(DWORD id, const wstring &name, File *folder, TexturePack *fallback)
	: AbstractTexturePack(id, folder, name, fallback)
{
	// Parse Bedrock JSON files from the folder
	m_bedrockPack.load(folder->getPath());

	// Load pack metadata (icon, name, description)
	loadIcon();
	loadName();
	loadDescription();
}

void BedrockFolderPack::unload(Textures *textures)
{
	AbstractTexturePack::unload(textures);
}

InputStream *BedrockFolderPack::getResourceImplementation(const wstring &name)
{
	wstring fullPath = getPath() + name;
	InputStream *resource = InputStream::getResourceAsStream(fullPath);
	return resource;
}

bool BedrockFolderPack::hasFile(const wstring &name)
{
	File f(getPath() + name);
	return f.exists() && f.isFile();
}

wstring BedrockFolderPack::getPath(bool bTitleUpdateTexture)
{
	return file->getPath() + L"\\";
}
