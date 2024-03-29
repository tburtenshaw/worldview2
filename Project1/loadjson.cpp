#include <iostream>
//#include <tchar.h>
#include <Windows.h>
#include <chrono>

#include "header.h"
#include "loadjson.h"
#include <filesystem>

struct json_reader_state {
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

};

int ProcessJsonBuffer(const char* buffer, const unsigned long buffersize, json_reader_state* jsr, std::vector<Location>& loc) {

	for (unsigned long i = 0; i < buffersize; i++) {
		char c = buffer[i];


		//String reading
		if ((c == '\"') && (!jsr->readingstring)) {	//faster with the c comparitor first
			jsr->readingstring = true;
			jsr->distancealongbuffer = 0;
		}
		else if ((c == '\"') && (jsr->readingstring) && (!jsr->escaped)) {
			jsr->readingstring = false;
			jsr->buffer[jsr->distancealongbuffer] = 0;
			if (!jsr->readingvalue) {
				memcpy(jsr->name, jsr->buffer, 16);
				jsr->name[16] = 0;

				//specifically check that we're reading name= "locations"
				if (jsr->locationsdepth == 0) {
					if (jsr->buffer[9] == 0) { //check it's 9 in length
						if (!strcmp(jsr->buffer,"locations")) {
							jsr->locationsdepth = jsr->hierarchydepth + 1; //i get this so I can just ignore all other info at the moment
							//printf("LOCATIONS found depth: %i", jsr->locationsdepth);
						}

					}
				}

			}
			else {
				AssignValueToName(jsr);
				jsr->readingvalue = false;
			}
			jsr->distancealongbuffer = 0;

		}
		else if ((c == '\\') && !jsr->escaped) {
			jsr->escaped = true;
		}
		else if (jsr->readingstring) {	//if nothing special, write the character to the buffer
			jsr->buffer[jsr->distancealongbuffer] = c;
			jsr->distancealongbuffer++;
			jsr->escaped = false;
		}

		//if we're not reading a string
		if (!jsr->readingstring) {

			if (c == ':') {
				jsr->readingvalue = true;
			}
			//else if (c == ',') {
			//	jsr->readingvalue = 0;
			//}

			else if (!jsr->readingnumber && jsr->readingvalue)
			{
				if (((c >= '0') && (c <= '9')) || (c == '-')) {	//leading zeros aren't strictly allowed in json, but i'll accept them
					jsr->readingnumber = 1;
					jsr->distancealongbuffer = 0;
				}
			}

			if (jsr->readingnumber) {
				if (((c >= '0') && (c <= '9')) || (c == '-') || (c == 'E') || (c == 'e') || (c == '+') || (c == '.')) {
					jsr->buffer[jsr->distancealongbuffer] = c;
					jsr->distancealongbuffer++;
				}
				else
				{
					jsr->readingnumber = false;
					jsr->buffer[jsr->distancealongbuffer] = 0;
					AssignValueToName(jsr);
					jsr->distancealongbuffer = 0;
					jsr->readingvalue = false;
				}

			}

			//I'm currently ignoring square brackets, instead just using the {} hierarchy
			/*
			if (c == '[') {
				jsr->arraydepth++;
			}
			if (c == ']') {
				jsr->arraydepth--;
			}
			*/


			if (c == '{') {
				jsr->hierarchydepth++;
				jsr->readingvalue = false;
		//		printf("\n%i", jsr->hierarchydepth);
		//		for (int e = 0; e < jsr->hierarchydepth; e++) {
			//		printf(" ");
			//	}
			}
			else if (c == '}') {
				if (jsr->hierarchydepth < jsr->locationsdepth) {	//if we've closed the locations data, then we don't need to continue
					return 1;
				}

				if (jsr->hierarchydepth == jsr->locationsdepth) {	//if we're closing up a location then write the location
					
					loc.emplace_back(jsr->location);

					//reset to defaults
					jsr->location.altitude = -1;

					//printf("Loc:%i", jsr->locationnumber);
				}
				jsr->hierarchydepth--;

				//printf("\n");
			}

		}

	}

	return 0;
}

double fast_strtolatlongdouble(char* str)
{
	long val = 0;
	int posneg = 1;
	double d;

	if (str[0] == '-') {
		posneg = -1;
		str++;
	}

	while (*str) {
		val = val * 10 + (*str++ - '0');
	}

	if (posneg < 1) { val = 0 - val; }
	
	d = val;
	return d/10000000.0;
}

float fast_strtolatlong(char* str)
{
	long val = 0;
	int posneg = 1;
	float f;

	if (str[0] == '-') {
		posneg = -1;
		str++;
	}

	while (*str) {
		val = val * 10 + (*str++ - '0');
	}

	if (posneg < 1) { val = 0 - val; }

	f = (float)val;

	return f / 10000000.0f;
}


unsigned long fast_strtotimestamp(char* str)
{
	unsigned long val = 0;

	while (*str) {
		val = val * 10 + (*str++ - '0');
		if (val > 1000000000) {	//once we're past Sep 2001, we'll stop, it should be right order of magnitude, Location History wasn't around
			return val;
		}
	}

	return val;
}

long fast_strtol(char* str)
{
	unsigned long val = 0;

	while (*str) {
		val = val * 10 + (*str++ - '0');
	}
	return val;
}

long long fast_strto64(char* str)
{
	unsigned long long val = 0;

	while (*str) {
		val = val * 10 + (*str++ - '0');
	}
	return val;
}



long timestampToLong(char* str)
{
	//Takes form:
	// 2013-09-16T02:34:43.164Z
	// 000000000011111111112222
	// 012345678901234567890123
	
	unsigned long year;
	unsigned long month;
	unsigned long dayofmonth;

	year = (str[0] - '0') * 1000 + (str[1] - '0')*100+ (str[2] - '0')*10 + (str[3] - '0');
	//str[4] is '-'
	month = (str[5] - '0') * 10 + (str[6] - '0');
	dayofmonth = (str[8] - '0') * 10 + (str[9] - '0');
	
	unsigned long hour;
	unsigned long minute;
	unsigned long second_floored;
	//unsigned long decimalportionofseconds;

	hour = (str[11] - '0') * 10 + (str[12] - '0');
	minute = (str[14] - '0') * 10 + (str[15] - '0');
	second_floored = (str[17] - '0') * 10 + (str[18] - '0');


	unsigned long unixtimefromyear = 94694400 + ((year + 3) % 4) * 365 * 86400 + ((year - 1973) / 4) * 126230400;

	const unsigned int daystoaddpermonthminusone[12] = { 0,31,59,90,120,151,181,212,243,273,304,334};
	unsigned long unixtimefrommonth = daystoaddpermonthminusone[month - 1]* 86400;

	unsigned long unixtimefromday = (dayofmonth - 1) * 86400;

	unsigned long leapday=0;
	if (year % 4 == 0 && month > 2) {	//simplied, we don't need really historic dates, so divisible by 4 is fine.
		leapday = 86400	;
	}

	return unixtimefromyear+ unixtimefromday+ unixtimefrommonth + leapday+ hour*3600+minute*60+second_floored;
}

int AssignValueToName(json_reader_state* jsr)
{
//	printf("%s=%s\t", jsr->name, jsr->buffer);

	//use the long of the first four characters to speed things up
	long * firstlong = (long *)jsr->name;
	//printf("%s %x", jsr->name,firstlong[0]);


	switch (firstlong[0]) {
		
	case 0x656d6974:
		//printf("%i\n", jsr->hierarchydepth);
		jsr->location.hierarchy = jsr->hierarchydepth;

		if (!strcmp(jsr->name, "timestampMs")) {	//the old version had the milliseconds
			//printf("TS %s ", jsr->buffer);
			jsr->location.timestamp = fast_strtotimestamp(jsr->buffer);
	
			//printf("TS %i ", jsr->location.timestamp);
			return 1;
		}
		if (!strcmp(jsr->name, "timestamp")) {	//new version has format: 2013-09-16T02:34:43.164Z
			//printf("TS %s ", jsr->buffer);
			jsr->location.timestamp = timestampToLong(jsr->buffer);
			//printf("TS %i \n", jsr->location.timestamp);
			return 1;
		}
		break;
	case 0x6974616c:
		if (!strcmp(jsr->name, "latitudeE7")) {
			//	jsr->location.latitude = strtod(jsr->buffer, NULL) / 10000000.0;
			jsr->location.latitude = fast_strtolatlongdouble(jsr->buffer);
			return 1;
		}
		break;
	case 0x676e6f6c: //gnol
		if (!strcmp(jsr->name, "longitudeE7")) {
			jsr->location.longitude = fast_strtolatlongdouble(jsr->buffer);
			/*
			if (jsr->location.longitude != (double)((float)jsr->location.longitude)) {
				printf("%s %.7f, %.7f\n", jsr->buffer, jsr->location.longitude, (float)jsr->location.longitude);
			}
			*/
			return 1;
		}
		break;
	case 0x75636361:	//ucca (accu, backwards)
		if (!strcmp(jsr->name, "accuracy")) {
			//if (jsr->hierarchydepth>2)	printf("%i %s=%s (prev %i)\n", jsr->hierarchydepth,  jsr->name, jsr->buffer,jsr->location.accuracy);
			jsr->location.accuracy = fast_strtol(jsr->buffer);
			return 1;
		}
		break;
	case 0x69746c61:	//itla
		if (!strcmp(jsr->name, "altitude")) {
			jsr->location.altitude = fast_strtol(jsr->buffer);
			return 1;
		}
		break;
	case 0x6f6c6576:	//olev
		if (!strcmp(jsr->name, "velocity")) {
			jsr->location.velocity = fast_strtol(jsr->buffer);
			return 1;
		}
		break;
	case 0x64616568:
		if (!strcmp(jsr->name, "heading")) {
			jsr->location.heading = fast_strtol(jsr->buffer);
			return 1;
		}
		break;
	case 0x74726576:
		if (!strcmp(jsr->name, "verticalAccuracy")) {
			jsr->location.verticalaccuracy = fast_strtol(jsr->buffer);

			return 1;
		}
		break;
	case 0x72756f73: //ruos (source)
		if ((firstlong[1] & 0x00ff'ffff) == 0x00006563) {	//\0ec
			jsr->location.source = std::string(jsr->buffer, strlen(jsr->buffer));
		}
		break;
	case 0x69766564:
		//deviceTag //deviceDesignatio //deviceTimestamp
		if (!strcmp(jsr->name, "deviceTag")) {
			jsr->location.deviceTag = std::string(jsr->buffer, strlen(jsr->buffer));
		}
		//else
			//printf("%x %x %i %s=%s\n", firstlong[0], firstlong[1], jsr->hierarchydepth, jsr->name, jsr->buffer);
		break;
	case 0x65707974:
		//type
		if ((firstlong[1] & 0x00'00ff) == 0x00) {	//still need to check the next char is 0 (could have been a longer string starting in "type"
			jsr->location.type = std::string(jsr->buffer, strlen(jsr->buffer));
			
		}
	case 0x666e6f63:
		//confidence
	case 0x56746e69:
		//intVal
	case 0x65727473: 
		//strength
		break;
	case 0x0063616d:
		//mac // this is the decimal of the 12-digit hex number
		jsr->location.mac = fast_strto64(jsr->buffer);
		//printf("%x %x %i %s=%s\n", firstlong[0], firstlong[1], jsr->hierarchydepth, jsr->name, jsr->buffer);
	case 0x6d726f66:
		//formFactor
		break;
	case 0x656d616e:
		//name
		break;
	case 0x74616c70:
		if (!strcmp(jsr->name, "platformType")) {
			jsr->location.platformType = std::string(jsr->buffer, strlen(jsr->buffer));
		}
		return 1;
		//platformType
		
	case 0x74746162:
		//batteryCharging
	case 0x76726573:
		//serverTimestamp
	case 0x6f437369:
		//isConnected
	case 0x654c736f:
		//osLevel
	case 0x63616c70:
		//placeId
		break;
	case 0x71657266:
		//frequencyMhz
		if (!strcmp(jsr->name, "frequencyMhz")) {
			jsr->location.frequencyMhz = fast_strtol(jsr->buffer);
		}
		return 1;
		break;
	break;
	}

	

	return 0; //return 0 if we didn't use anything
}


void FileLoader::SetFullyLoaded(bool tf)
{
	fullyLoaded.store(tf);
}

bool FileLoader::IsFullyLoaded() const
{
	return fullyLoaded.load();
}

bool FileLoader::IsLoadingFile() const
{
	return loadingFile;
}

bool FileLoader::IsError() const
{
	return errorState;
}

std::string FileLoader::GetFilename() const
{
	int requiredSize = WideCharToMultiByte(CP_UTF8, 0, loadedLocationFilename.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (requiredSize == 0) return "";

	std::string result(requiredSize, 0);
	int actualSize = WideCharToMultiByte(CP_UTF8, 0, loadedLocationFilename.c_str(), -1, &result[0], requiredSize, nullptr, nullptr);
	if (actualSize == 0) return "";

	return result;
}

unsigned long FileLoader::GetFileSize() const
{
	return filesize;
}

unsigned long FileLoader::GetTotalBytesRead() const
{
	return totalbytesread;
}

float FileLoader::GetSecondsToLoad() const
{
	return secondsToLoad;
}

bool FileLoader::LoadJsonFile(LocationHistory * lh, void* jsonfile)
{
	json_reader_state jrs;
	memset(&jrs, 0, sizeof(jrs));

	constexpr int readBufferSize = 8192;	//seems most efficient buffer size (diminshing returns increasing this, plus can have on stack this size)
	char buffer[readBufferSize] = { 0 };

	auto start = std::chrono::high_resolution_clock::now();

	totalbytesread = 0;
	lh->locations.reserve(filesize / 512);	//there will be more locations than this as it seems that each location uses much less than 512 bytes (258-294 in my testing)


	unsigned long readbytes = 1;
	while (readbytes) {
		bool rf = ReadFile(jsonfile, buffer, readBufferSize - 1, &readbytes, NULL);
		if (rf == false) {
			printf("Failed reading the file.\n");
			return false;
		}
		int result = ProcessJsonBuffer(buffer, readbytes, &jrs, lh->locations);
		totalbytesread += readbytes;
		if (result) {
			break;
		}
	}

	auto stop = std::chrono::high_resolution_clock::now();
		
	auto duration = duration_cast<std::chrono::milliseconds>(stop - start);
	std::cout <<"Time to load: " << duration.count() << "ms.\n";
	secondsToLoad = static_cast<float>(duration.count())/1000.f;

	return 0;
}


bool FileLoader::LoadWVFormat(LocationHistory* lh, void* WVFfile)
{
	DWORD bytesRead;
	DWORD numberOfLocations;

	UINT32 magic;

	constexpr DWORD locLimit = 0x04000000;	//largest number of locations we'll allow

	LARGE_INTEGER largeFileSize;
	GetFileSizeEx(WVFfile, &largeFileSize);
	filesize = (unsigned long)largeFileSize.QuadPart;

	bool fileRead = ReadFile(WVFfile, &magic, 4, &bytesRead, NULL);
	printf("Magic: %i\n", magic);
	if (!fileRead || ((magic & 0x00FFFFFF) != 0x00465657) || (bytesRead != 4)) {
		printf("Couldn't read magic number.\n");
		return 1;
	}
	totalbytesread += bytesRead;

	fileRead = ReadFile(WVFfile, &numberOfLocations, 4, &bytesRead, NULL);
	if (!fileRead || (numberOfLocations > locLimit) || (bytesRead != 4) || (numberOfLocations * sizeof(WVFormat) != filesize - 8)) {
		printf("Location number likely incorrect.\n");
		return 1;
	}
	totalbytesread += bytesRead;

	constexpr unsigned int entriesToRead = 1024;
	WVFormat entry[entriesToRead];
	Location newLocation;

	lh->locations.reserve(numberOfLocations);

	DWORD bytesStillToRead = filesize - 8;

	while (bytesStillToRead) {
		bool fileRead = ReadFile(WVFfile, entry, sizeof(entry), &bytesRead, NULL);
		bytesStillToRead -= bytesRead;
		unsigned int entriesRead = bytesRead / sizeof(WVFormat);
		//printf("Read bytes: %i\n", bytesRead);
		if (fileRead) {
			for (unsigned int i = 0; i < entriesRead; i++) {
				newLocation.timestamp = entry[i].timestamp;
				newLocation.longitude = entry[i].lon;
				newLocation.latitude = entry[i].lat;
				lh->locations.emplace_back(newLocation);
				totalbytesread += bytesRead;
			}
		}
		else
		{
			printf("Error reading entries.");
			return false;
		}
	}

	return true;
}

bool FileLoader::OpenFile(LocationHistory &lh, std::wstring filename)
{
	HANDLE locationFileHandle;

	if (loadingFile) {	//we don't want to load if we're already loading something
		return false;
	}
	loadedLocationFilename = filename;

	locationFileHandle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (locationFileHandle == INVALID_HANDLE_VALUE) {
		loadingFile = false;
		SetFullyLoaded(false);
		errorState = true;
		return false;
	}

	lh.EmptyLocationInfo();	//close any existing file
	loadingFile = true;
	errorState = false;

	LARGE_INTEGER LIfilesize;
	GetFileSizeEx(locationFileHandle, &LIfilesize);
	filesize = LIfilesize.QuadPart;
	printf("File size: %i\n", filesize);

	std::string extension = std::filesystem::path(filename).extension().string();
	if (extension == ".json") {
		LoadJsonFile(&lh, locationFileHandle);
	}
	else if (extension == ".wvf") {
		LoadWVFormat(&lh, locationFileHandle);
	}
	CloseHandle(locationFileHandle);
	
	SetFullyLoaded(false);
	loadingFile = false;

	return true;
}

bool FileLoader::CloseFile()
{
	if (loadingFile) return false;	//we can't close while we're loading something (at least not at the moment)
	
	SetFullyLoaded(true);	//loaded with nothing
	loadingFile = false;

	filesize = 0;
	loadedLocationFilename = L"";

	return true;
}
