#include "stdafx.h"
#include "Language.h"
#include <cstdio>
#include <string>

Language *Language::singleton = new Language();

Language::Language()
{
}

void Language::Load(const std::string& langCode)
{
    m_entries.clear();

    // Helper lambda to load one .lang file
    auto loadFile = [&](const char* path) {
        FILE* f = fopen(path, "r");
        if (!f) return;
        char lineBuf[512];
        while (fgets(lineBuf, sizeof(lineBuf), f)) {
            // Strip trailing newline/CR
            size_t len = strlen(lineBuf);
            while (len > 0 && (lineBuf[len-1] == '\n' || lineBuf[len-1] == '\r'))
                lineBuf[--len] = '\0';
            // Skip blank lines and comments
            if (len == 0 || lineBuf[0] == '#') continue;
            // Find '='
            char* eq = strchr(lineBuf, '=');
            if (!eq) continue;
            *eq = '\0';
            const char* keyUtf8 = lineBuf;
            const char* valUtf8 = eq + 1;
            // Convert key (ASCII) to wstring
            std::wstring key(keyUtf8, keyUtf8 + strlen(keyUtf8));
            // Convert value UTF-8 to wstring using MultiByteToWideChar
            int valLen = (int)strlen(valUtf8);
            if (valLen == 0) {
                m_entries[key] = std::wstring();
            } else {
                int wlen = MultiByteToWideChar(CP_UTF8, 0, valUtf8, valLen, nullptr, 0);
                if (wlen > 0) {
                    std::wstring val(wlen, L'\0');
                    MultiByteToWideChar(CP_UTF8, 0, valUtf8, valLen, &val[0], wlen);
                    m_entries[key] = val;
                }
            }
        }
        fclose(f);
    };

    // Load main lang file
    std::string mainPath = "Data/resource_packs/vanilla/texts/" + langCode + ".lang";
    loadFile(mainPath.c_str());

    // Load custom UI strings override (always)
    loadFile("Data/ui/ui_strings.lang");
}

Language *Language::getInstance()
{
	return singleton;
}

/* 4J Jev, creates 2 identical functions.
wstring Language::getElement(const wstring& elementId)
{
	return elementId;
} */

wstring Language::getElement(const wstring elementId, ...)
{
	va_list args;
	va_start(args, elementId);
	return getElement(elementId, args);
}

wstring Language::getElement(const wstring& elementId, va_list args)
{
    auto it = m_entries.find(elementId);
    if (it != m_entries.end())
        return it->second;
    return elementId;
}

wstring Language::getElementName(const wstring& elementId)
{
    auto it = m_entries.find(elementId);
    if (it != m_entries.end())
        return it->second;
    return elementId;
}

wstring Language::getElementDescription(const wstring& elementId)
{
    auto it = m_entries.find(elementId);
    if (it != m_entries.end())
        return it->second;
    return elementId;
}