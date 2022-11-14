#pragma once
#include <vector>
using namespace std;

constexpr int maxJsonString = 1024;

typedef struct json_reader_state {
	int hierarchydepth;	//counts the level of { }s
	bool escaped;
	bool readingvalue; //i.e. we're not reading a name
	bool readingstring;
	bool readingnumber;
	int distancealongbuffer;
	char buffer[maxJsonString];	//what both numbers and strings are read into
	int arraydepth;	//counts the [ ]s

	char name[maxJsonString];
	//char value[MAX_JSON_STRING]; //this can just stay as the buffer
	int locationsdepth;

	Location location;

	//Location oldlocation; //to compare to previous to see if crosses earth the other way around
	

} JSON_READER_STATE;

int ProcessJsonBuffer(const char* buffer, const unsigned long buffersize, JSON_READER_STATE* jsr, vector<Location> &loc);
int AssignValueToName(JSON_READER_STATE* jsr);

int LoadJsonFile(LocationHistory* lh, HANDLE jsonfile);
int LoadWVFormat(LocationHistory* lh, HANDLE WVFfile);
