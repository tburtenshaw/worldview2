#pragma once
#include <filesystem>

class FileManager {
private:
	static std::filesystem::path backgroundImagePath;
public:
	static std::string GetBackgroundImageFilename();
	static void SetBackgroundImageFilename(const std::string& filename);
	static void SetBackgroundImageFilename(const std::wstring& filename);
};

