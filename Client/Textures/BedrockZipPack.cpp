#include "stdafx.h"
#include "BedrockZipPack.h"
#include "Common/minizip/unzip.h"
#include "../World/IO/ByteArrayInputStream.h"
#include "../World/IO/File.h"
#include <codecvt>
#include <fstream>
#include <Shlobj.h>

static std::string toNarrow(const std::wstring &s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(s);
}

static std::wstring toWide(const std::string &s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(s);
}

// Create a unique temp directory for extracting JSON metadata
static wstring createTempDir()
{
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);

	wstring dir = wstring(tempPath) + L"BedrockZipPack_" + to_wstring(GetTickCount64());
	CreateDirectoryW(dir.c_str(), NULL);
	return dir;
}

static void deleteDirectoryRecursive(const wstring &path)
{
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW((path + L"\\*").c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do
	{
		wstring name = fd.cFileName;
		if (name == L"." || name == L"..") continue;

		wstring full = path + L"\\" + name;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			deleteDirectoryRecursive(full);
		else
			DeleteFileW(full.c_str());
	} while (FindNextFileW(hFind, &fd));

	FindClose(hFind);
	RemoveDirectoryW(path.c_str());
}

BedrockZipPack::BedrockZipPack(DWORD id, File *zipFile, TexturePack *fallback)
	: AbstractTexturePack(id, zipFile, zipFile->getName(), fallback)
	, m_zipHandle(NULL)
	, m_zipOpen(false)
{
	m_zipPath = zipFile->getPath();
	m_tempDir = createTempDir();

	openZip();

	// Extract JSON metadata files to temp dir for BedrockResourcePack parsing
	const char *jsonFiles[] = {
		"manifest.json",
		"textures/terrain_texture.json",
		"textures/item_texture.json",
		"textures/flipbook_textures.json"
	};

	for (int i = 0; i < 4; i++)
	{
		byteArray data = extractFile(jsonFiles[i]);
		if (data.data != NULL && data.length > 0)
		{
			// Build output path, creating subdirectories as needed
			wstring outPath = m_tempDir + L"\\" + toWide(jsonFiles[i]);

			// Replace forward slashes with backslashes
			for (size_t j = 0; j < outPath.size(); j++)
			{
				if (outPath[j] == L'/') outPath[j] = L'\\';
			}

			// Create subdirectory if needed
			size_t lastSlash = outPath.rfind(L'\\');
			if (lastSlash != wstring::npos)
			{
				wstring subDir = outPath.substr(0, lastSlash);
				CreateDirectoryW(subDir.c_str(), NULL);
			}

			// Write extracted data to temp file
			std::ofstream out(toNarrow(outPath), std::ios::binary);
			if (out.is_open())
			{
				out.write((const char *)data.data, data.length);
				out.close();
			}

			delete[] data.data;
		}
	}

	m_bedrockPack.load(m_tempDir);

	loadIcon();
	loadName();
	loadDescription();
}

BedrockZipPack::~BedrockZipPack()
{
	closeZip();

	if (!m_tempDir.empty())
		deleteDirectoryRecursive(m_tempDir);
}

void BedrockZipPack::unload(Textures *textures)
{
	AbstractTexturePack::unload(textures);
	closeZip();
}

void BedrockZipPack::openZip()
{
	if (m_zipOpen) return;

	string narrowPath = toNarrow(m_zipPath);
	m_zipHandle = unzOpen(narrowPath.c_str());
	m_zipOpen = (m_zipHandle != NULL);
}

void BedrockZipPack::closeZip()
{
	if (m_zipOpen && m_zipHandle != NULL)
	{
		unzClose((unzFile)m_zipHandle);
		m_zipHandle = NULL;
		m_zipOpen = false;
	}
}

byteArray BedrockZipPack::extractFile(const string &entryName)
{
	byteArray result;
	result.data = NULL;
	result.length = 0;

	if (!m_zipOpen || m_zipHandle == NULL) return result;

	if (unzLocateFile((unzFile)m_zipHandle, entryName.c_str(), 2) != UNZ_OK)
		return result;

	unz_file_info fileInfo;
	if (unzGetCurrentFileInfo((unzFile)m_zipHandle, &fileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
		return result;

	if (unzOpenCurrentFile((unzFile)m_zipHandle) != UNZ_OK)
		return result;

	unsigned int size = (unsigned int)fileInfo.uncompressed_size;
	byte *buffer = new byte[size];
	int bytesRead = unzReadCurrentFile((unzFile)m_zipHandle, buffer, size);
	unzCloseCurrentFile((unzFile)m_zipHandle);

	if (bytesRead <= 0)
	{
		delete[] buffer;
		return result;
	}

	result.data = buffer;
	result.length = (unsigned int)bytesRead;
	return result;
}

InputStream *BedrockZipPack::getResourceImplementation(const wstring &name)
{
	openZip();

	// Strip leading slash or backslash
	wstring stripped = name;
	while (!stripped.empty() && (stripped[0] == L'/' || stripped[0] == L'\\'))
		stripped = stripped.substr(1);

	// Convert backslashes to forward slashes for ZIP entry lookup
	for (size_t i = 0; i < stripped.size(); i++)
	{
		if (stripped[i] == L'\\') stripped[i] = L'/';
	}

	string narrow = toNarrow(stripped);
	byteArray data = extractFile(narrow);

	if (data.data == NULL || data.length == 0)
		return NULL;

	// ByteArrayInputStream takes ownership of the byteArray data
	return new ByteArrayInputStream(data);
}

bool BedrockZipPack::hasFile(const wstring &name)
{
	openZip();
	if (!m_zipOpen || m_zipHandle == NULL) return false;

	// Strip leading slash or backslash
	wstring stripped = name;
	while (!stripped.empty() && (stripped[0] == L'/' || stripped[0] == L'\\'))
		stripped = stripped.substr(1);

	// Convert backslashes to forward slashes
	for (size_t i = 0; i < stripped.size(); i++)
	{
		if (stripped[i] == L'\\') stripped[i] = L'/';
	}

	string narrow = toNarrow(stripped);
	return (unzLocateFile((unzFile)m_zipHandle, narrow.c_str(), 2) == UNZ_OK);
}
