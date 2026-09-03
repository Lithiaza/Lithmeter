#pragma once
#include "pch.h"
#include <unordered_map>

#define LANGMANAGER Language::getInstance()

class Language : public Singleton<Language>
{

private:
	std::unordered_map<std::string, std::string> _textList;
	char _currentLang[128] = { 0 };
	std::vector<std::string> _notFoundText;

public:
	Language() : _currentLang("zh_tw.json") {}

	const char _langFolder[6] = "Lang/";

	const char* GetCurrentLang() const
	{
		return _currentLang;
	}
	DWORD SetCurrentLang(const char* langFile);
	const std::string_view GetText(const char* text, std::unordered_map<std::string, std::string>* vector = nullptr);
	std::unordered_map<std::string, std::string> GetAllLangFile();
	auto GetLangFile(const char* langFile, bool outputERROR = true);
	std::unordered_map<std::string, std::string> MapLangData(const char* langFile, bool useReplace = true);

private:
	std::filesystem::path GetLangDirectory() const;
};
