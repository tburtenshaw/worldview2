#include "filemanager.h"
#include <windows.h>

//Defaults/preloads
std::filesystem::path FileManager::backgroundImagePath = "d:/world.200409.3x4096x2048.png";

std::string FileManager::GetBackgroundImageFilename()
{
	return backgroundImagePath.string();
}

void FileManager::SetBackgroundImageFilename(const std::string& filename)
{
	backgroundImagePath = filename;
}

void FileManager::SetBackgroundImageFilename(const std::wstring& filename)
{
	int requiredSize = WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string convertedFilename(requiredSize, 0);
	WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, &convertedFilename[0], requiredSize, nullptr, nullptr);
	backgroundImagePath = filename;
	
}
