#pragma once
#include <unordered_map>
#include <string>

class Language
{
private:
	static Language *singleton;
	std::unordered_map<std::wstring, std::wstring> m_entries;
public:
	Language();
    static Language *getInstance();
    void Load(const std::string& langCode);
    std::wstring getElement(const std::wstring elementId, ...);
	std::wstring getElement(const std::wstring& elementId, va_list args);
    std::wstring getElementName(const std::wstring& elementId);
    std::wstring getElementDescription(const std::wstring& elementId);
};