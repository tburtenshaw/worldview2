#pragma once
#include <vector>
using namespace std;

typedef struct json_reader_state {
	int hierarchydepth;	//counts the level of { }s
	int escaped;
	int readingvalue; //i.e. we're not reading a name
	int readingstring;
	int readingnumber;
	int distancealongbuffer;
	char buffer[MAX_JSON_STRING];	//what both numbers and strings are read into
	int arraydepth;	//counts the [ ]s

	char name[MAX_JSON_STRING];
	//char value[MAX_JSON_STRING]; //this can just stay as the buffer

	int locationsdepth;
	long locationnumber;

	Location location;

	Location oldlocation; //to compare to previous to see if crosses earth the other way around
	

} JSON_READER_STATE;

int ProcessJsonBuffer(char* buffer, unsigned long buffersize, JSON_READER_STATE* jsr, vector<Location> &loc, LocationHistory* lh);
int AssignValueToName(JSON_READER_STATE* jsr);

int LoadJsonFile(LocationHistory* lh, HANDLE jsonfile);
int LoadWVFormat(LocationHistory* lh, HANDLE WVFfile);
