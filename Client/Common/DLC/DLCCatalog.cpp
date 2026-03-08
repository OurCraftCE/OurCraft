#include "stdafx.h"
#include "DLCCatalog.h"

DLCCatalog::DLCCatalog()
{
}

DLCCatalog::~DLCCatalog()
{
}

wstring DLCCatalog::TrimWhitespace(const wstring &str)
{
	size_t start = str.find_first_not_of(L" \t\r\n");
	if (start == wstring::npos) return L"";
	size_t end = str.find_last_not_of(L" \t\r\n");
	return str.substr(start, end - start + 1);
}

bool DLCCatalog::ParseLine(const wstring &line, wstring &key, wstring &value)
{
	size_t eq = line.find(L'=');
	if (eq == wstring::npos) return false;
	key = TrimWhitespace(line.substr(0, eq));
	value = TrimWhitespace(line.substr(eq + 1));
	return true;
}

bool DLCCatalog::LoadFromFile(const wstring &filePath)
{
	m_entries.clear();

	FILE *fp = NULL;
	_wfopen_s(&fp, filePath.c_str(), L"r, ccs=UTF-8");
	if (!fp)
	{
		return false;
	}

	WCHAR lineBuf[1024];
	DLCCatalogEntry *currentEntry = NULL;

	while (fgetws(lineBuf, 1024, fp))
	{
		wstring line = TrimWhitespace(lineBuf);
		if (line.empty() || line[0] == L';' || line[0] == L'#')
			continue;

		// Section header: [PackID]
		if (line[0] == L'[' && line[line.length() - 1] == L']')
		{
			DLCCatalogEntry entry;
			entry.id = line.substr(1, line.length() - 2);
			entry.installed = false;
			entry.sortIndex = 0;
			m_entries.push_back(entry);
			currentEntry = &m_entries.back();
			continue;
		}

		if (currentEntry == NULL) continue;

		wstring key, value;
		if (!ParseLine(line, key, value)) continue;

		if (key == L"name")             currentEntry->name = value;
		else if (key == L"type")        currentEntry->type = value;
		else if (key == L"description") currentEntry->description = value;
		else if (key == L"version")     currentEntry->version = value;
		else if (key == L"price")       currentEntry->price = value;
		else if (key == L"thumbnail")   currentEntry->thumbnailPath = value;
		else if (key == L"datapath")    currentEntry->dataPath = value;
		else if (key == L"sortindex")   currentEntry->sortIndex = (unsigned int)_wtoi(value.c_str());
	}

	fclose(fp);
	return true;
}

bool DLCCatalog::Refresh(const wstring &catalogPath, const wstring &installedPath)
{
	if (!LoadFromFile(catalogPath))
		return false;
	CheckInstalledStatus(installedPath);
	return true;
}

void DLCCatalog::CheckInstalledStatus(const wstring &installedPath)
{
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		wstring checkPath = installedPath + L"\\" + m_entries[i].id;
		DWORD attrib = GetFileAttributesW(checkPath.c_str());
		m_entries[i].installed = (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
	}
}

int DLCCatalog::GetEntryCount() const
{
	return (int)m_entries.size();
}

const DLCCatalogEntry *DLCCatalog::GetEntry(int index) const
{
	if (index < 0 || index >= (int)m_entries.size()) return NULL;
	return &m_entries[index];
}

const DLCCatalogEntry *DLCCatalog::FindEntry(const wstring &id) const
{
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		if (m_entries[i].id == id)
			return &m_entries[i];
	}
	return NULL;
}

int DLCCatalog::GetEntryCountByType(eDLCContentType type) const
{
	int count = 0;
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		if (m_entries[i].GetContentType() == type)
			count++;
	}
	return count;
}

const DLCCatalogEntry *DLCCatalog::GetEntryByType(eDLCContentType type, int index) const
{
	int count = 0;
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		if (m_entries[i].GetContentType() == type)
		{
			if (count == index)
				return &m_entries[i];
			count++;
		}
	}
	return NULL;
}

void DLCCatalog::MarkInstalled(const wstring &id, bool installed)
{
	for (size_t i = 0; i < m_entries.size(); i++)
	{
		if (m_entries[i].id == id)
		{
			m_entries[i].installed = installed;
			return;
		}
	}
}
