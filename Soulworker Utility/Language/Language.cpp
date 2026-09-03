#include "pch.h"
#include "Language.h"

std::filesystem::path Language::GetLangDirectory() const
{
	// Prefer the working-directory path for development, then fall back to
	// the directory next to the executable for installed/relaunched builds.
	std::filesystem::path relative(_langFolder);
	std::error_code ec;
	if (std::filesystem::is_directory(relative, ec))
		return relative;

	wchar_t modulePath[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameW(nullptr, modulePath, ARRAYSIZE(modulePath));
	if (length > 0 && length < ARRAYSIZE(modulePath)) {
		std::filesystem::path moduleLang = std::filesystem::path(modulePath).parent_path() / L"Lang";
		if (std::filesystem::is_directory(moduleLang, ec))
			return moduleLang;
	}

	return relative;
}

auto Language::GetLangFile(const char* langFile, bool outputERROR)
{
	if (langFile == nullptr || langFile[0] == '\0')
		return json();

	const std::filesystem::path path = GetLangDirectory() / std::filesystem::path(langFile);

	json j;

	do
	{
		// get raw data
		std::string langRaw;
		if (!file_contents(path, &langRaw)) {
			if (outputERROR)
				LogInstance.WriteLog("[Language::SetCurrentLang] Lang file %s not found at %s.", langFile, path.string().c_str());
			break;
		}
		// Some language editors save UTF-8 with a BOM. Strip it before handing
		// the document to the JSON parser so every bundled file behaves alike.
		if (langRaw.size() >= 3 &&
			static_cast<unsigned char>(langRaw[0]) == 0xEF &&
			static_cast<unsigned char>(langRaw[1]) == 0xBB &&
			static_cast<unsigned char>(langRaw[2]) == 0xBF) {
			langRaw.erase(0, 3);
		}

		// parse raw to json
		j = json::parse(langRaw);
		if (j.empty()) {
			if (outputERROR)
				LogInstance.WriteLog("[Language::SetCurrentLang] Lang file %s is empty.", langFile);
			break;
		}

	} while (false);

	return j;
}

std::unordered_map<std::string, std::string> Language::MapLangData(const char* langFile, bool useReplace)
{
	std::unordered_map<std::string, std::string> list;

	// get json data
	auto langData = GetLangFile(langFile);
	if (!langData.empty())
	{
		for (json::iterator itr = langData.begin(); itr != langData.end(); itr++)
			list[itr.key()] = itr.value().get<std::string>();

		if (useReplace)
		{
			try {
				auto replaceData = GetLangFile("replace.lang", false);
				if (!replaceData.empty())
				{
					for (json::iterator itr = replaceData.begin(); itr != replaceData.end(); itr++)
						list[itr.key()] = itr.value().get<std::string>();
				}
			}
			catch (...) {
				LogInstance.WriteLog("[Language::MapLangData] Load replace.lang failed.");
			}
		}
	}

	return list;
}

DWORD Language::SetCurrentLang(const char* langFile)
{
	if (langFile == nullptr || langFile[0] == '\0')
		return ERROR_INVALID_PARAMETER;

	DWORD error = ERROR_SUCCESS;
	LogInstance.WriteLog("[Language::SetCurrentLang] %s", langFile);
	do {
		std::unordered_map<std::string, std::string> newLang;

		// get json data
		try {
			newLang = MapLangData(langFile, true);
			if (newLang.empty()) {
				error = ERROR_NOT_FOUND;
				break;
			}
		}
		catch (...)
		{
			LogInstance.WriteLog("[Language::SetCurrentLang] Load lang error.");
			error = ERROR_ACCESS_DENIED;
			break;
		}

		// set current lang
		strcpy_s(_currentLang, langFile);

		_textList = newLang;

		_notFoundText.clear();

	} while (false);

	return error;
}

const std::string_view Language::GetText(const char* text, std::unordered_map<std::string, std::string>* vector) // this code is fucking awful but I dont care enough to refactor
{
	if (vector == nullptr)
		vector = &_textList;

	if (vector->find(text) == vector->end()) {
		std::string_view findStr(text);
		if (std::find(_notFoundText.begin(), _notFoundText.end(), findStr) == _notFoundText.end())
		{
			LogInstance.WriteLog("[Language::GetText] Lang text %s not found.", text);
			_notFoundText.emplace_back(findStr);
		}
		return text;
	}

	return vector->at(text);
}

std::unordered_map<std::string, std::string> Language::GetAllLangFile()
{
	std::unordered_map<std::string, std::string> list;

	try {
		for (const auto& p : std::filesystem::directory_iterator(GetLangDirectory())) {
			if (!p.is_regular_file() || p.path().extension().string() != ".json")
				continue;

			const std::string fileName = p.path().filename().string();
			try {
				auto langData = MapLangData(fileName.c_str());
				if (!langData.empty()) {
					const std::string langName = std::string(GetText("STR_LANG_NAME", &langData));
					list.emplace(fileName, langName);
				}
			}
			catch (...) {
				LogInstance.WriteLog("[Language::GetAllLangFile] Load lang file %s failed.", fileName.c_str());
			}
		}
	}
	catch (...) {
		LogInstance.WriteLog("[Language::GetAllLangFile] Language directory %s is unavailable.", GetLangDirectory().string().c_str());
	}

	return list;
}
