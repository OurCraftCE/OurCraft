#pragma once

#include <string>
#include <vector>
using namespace std;

// Catalog entry for a DLC pack available for local install
struct DLCCatalogEntry
{
	wstring id;              // unique pack identifier e.g. "SkinPack1"
	wstring name;            // display name
	wstring description;     // sell-text description
	wstring type;            // "skin", "texture", "mashup"
	wstring version;         // pack version string
	wstring price;           // display price (informational only for local installs)
	wstring thumbnailPath;   // relative path to thumbnail image
	wstring dataPath;        // relative path to pack data folder
	bool    installed;       // whether already installed
	unsigned int sortIndex;  // display sort order

	eDLCContentType GetContentType() const
	{
		if (type == L"skin")    return e_DLC_SkinPack;
		if (type == L"texture") return e_DLC_TexturePacks;
		if (type == L"mashup")  return e_DLC_MashupPacks;
		return e_DLC_NotDefined;
	}
};

class DLCCatalog
{
public:
	DLCCatalog();
	~DLCCatalog();

	// Load catalog from an INI-style text file
	// Format:
	//   [PackID]
	//   name=Display Name
	//   type=skin
	//   description=Some description text
	//   version=1.0
	//   price=Free
	//   thumbnail=dlc/available/PackID/thumbnail.png
	//   datapath=dlc/available/PackID
	//   sortindex=100
	bool LoadFromFile(const wstring &filePath);

	// Reload catalog and refresh installed status
	bool Refresh(const wstring &catalogPath, const wstring &installedPath);

	int GetEntryCount() const;
	const DLCCatalogEntry *GetEntry(int index) const;
	const DLCCatalogEntry *FindEntry(const wstring &id) const;

	// Get entries filtered by content type
	int GetEntryCountByType(eDLCContentType type) const;
	const DLCCatalogEntry *GetEntryByType(eDLCContentType type, int index) const;

	void MarkInstalled(const wstring &id, bool installed);

private:
	vector<DLCCatalogEntry> m_entries;

	// Parse helpers
	static wstring TrimWhitespace(const wstring &str);
	static bool ParseLine(const wstring &line, wstring &key, wstring &value);
	void CheckInstalledStatus(const wstring &installedPath);
};
