#pragma once
#include <vector>
#include <thread>
//#include "header.h"
class LocationHistory;


constexpr int maxJsonString = 1024;

struct json_reader_state;	//forward dec - see the cpp file

int ProcessJsonBuffer(const char* buffer, const unsigned long buffersize, json_reader_state* jsr, std::vector<Location> &loc);
int AssignValueToName(json_reader_state* jsr);

class FileLoader {
public:
	// Constructor
	FileLoader()
		: loadedLocationFilename(L""),      // Initialize std::wstring to empty string
		filesize(0),       
		secondsToLoad(0.0f), 
		fullyLoaded(false),
		loadingFile(false),
		errorState(false),
		totalbytesread(0)
	{	}

	void SetFullyLoaded(bool tf);	//should rename and move to LH, as this means it's been sorted, LODs generated etc.
	bool IsFullyLoaded() const;
	bool IsLoadingFile() const;
	bool IsError() const;

	std::string GetFilename() const;
	unsigned long GetFileSize() const;
	unsigned long GetTotalBytesRead() const;
	float GetSecondsToLoad() const;
	
	bool LoadJsonFile(LocationHistory* lh, void* jsonfile);
	bool LoadWVFormat(LocationHistory* lh, void* WVFfile);
	
	bool OpenFile(LocationHistory &lh, std::wstring filename);
	bool CloseFile();
private:
	std::wstring loadedLocationFilename;
	unsigned long filesize;
	float secondsToLoad;


	
	std::atomic<bool> fullyLoaded;
	std::atomic<bool> loadingFile;
	std::atomic<bool> errorState;

	std::atomic<unsigned long> totalbytesread;
};