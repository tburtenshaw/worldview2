#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <chrono>

#include "header.h"
#include "loadjson.h"
#include "heatmap.h"

int ProcessJsonBuffer(const char* buffer, const unsigned long buffersize, JSON_READER_STATE* jsr, vector<Location>& loc) {

	//char c;
#define c buffer[i]

	for (unsigned long i = 0; i < buffersize; i++) {
		//c = buffer[i]; //believe it or not, this makes a small bit of difference, so I've made it a define


		//String reading
		if ((c == '\"') && (!jsr->readingstring)) {
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
					
					//reset to defaults
					jsr->location.altitude = -1;
					loc.emplace_back(jsr->location);
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

int AssignValueToName(JSON_READER_STATE* jsr)
{
	//printf("%s=%s ", jsr->name, jsr->buffer);
	

	//use the long of the first four characters to speed things up
	long * firstlong = (long *)jsr->name;
	//printf("%s %x", jsr->name,firstlong[0]);


	switch (firstlong[0]) {
		
	case 0x656d6974:
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

	case 0x6974616c:
		if (!strcmp(jsr->name, "latitudeE7")) {
			//	jsr->location.latitude = strtod(jsr->buffer, NULL) / 10000000.0;
			jsr->location.latitude = fast_strtolatlongdouble(jsr->buffer);
			return 1;
		}
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
	case 0x75636361:	//ucca (accu, backwards)
		if (!strcmp(jsr->name, "accuracy")) {
			jsr->location.accuracy = fast_strtol(jsr->buffer);
			return 1;
		}
	case 0x69746c61:	//itla
		if (!strcmp(jsr->name, "altitude")) {
			jsr->location.altitude = fast_strtol(jsr->buffer);
			return 1;
		}

	case 0x6f6c6576:	//olev
		if (!strcmp(jsr->name, "velocity")) {
			jsr->location.velocity = fast_strtol(jsr->buffer);
			return 1;
		}

	case 0x64616568:
		if (!strcmp(jsr->name, "heading")) {
			jsr->location.heading = fast_strtol(jsr->buffer);
			return 1;
		}

	case 0x74726576:
		if (!strcmp(jsr->name, "verticalaccuracy")) {
			jsr->location.verticalaccuracy = fast_strtol(jsr->buffer);
			return 1;
		}
	}
	return 0; //return 0 if we didn't use anything
}


int LoadJsonFile(LocationHistory * lh, HANDLE jsonfile)
{
	JSON_READER_STATE jrs;
	memset(&jrs, 0, sizeof(jrs));

	constexpr int readBufferSize = 8192;	//seems most efficient buffer size (diminshing returns increasing this, plus can have on stack this size)
	char buffer[readBufferSize] = { 0 };

	auto start = std::chrono::high_resolution_clock::now();

	lh->totalbytesread = 0;
	lh->locations.reserve(lh->filesize / 512);	//there will be more locations than this as it seems that each location uses much less than 512 bytes (258-294 in my testing)


	unsigned long readbytes = 1;
	while (readbytes) {
		bool rf = ReadFile(jsonfile, buffer, readBufferSize - 1, &readbytes, NULL);
		if (rf == false) {
			printf("Failed reading the file.\n");
			return 1;
		}
		int result = ProcessJsonBuffer(buffer, readbytes, &jrs, lh->locations);
		lh->totalbytesread += readbytes;
		if (result) {
			readbytes = 0;	//trick the loading loop into ending
		}
	}

	auto stop = std::chrono::high_resolution_clock::now();
		
	auto duration = duration_cast<std::chrono::microseconds>(stop - start);
	std::cout <<"Time to load: " << duration.count() << "us.\n";
}


int LoadWVFormat(LocationHistory* lh, HANDLE WVFfile)
{
	DWORD bytesRead;
	DWORD numberOfLocations;

	UINT32 magic;

	constexpr DWORD locLimit = 0x04000000;	//largest number of locations we'll allow

	LARGE_INTEGER filesize;
	GetFileSizeEx(WVFfile, &filesize);
	lh->filesize = (unsigned long)filesize.QuadPart;

	bool fileRead = ReadFile(WVFfile, &magic, 4, &bytesRead, NULL);
	printf("Magic: %i\n", magic);
	if (!fileRead || ((magic & 0x00FFFFFF) != 0x00465657) || (bytesRead != 4)) {
		printf("Couldn't read magic number.\n");
		return 1;
	}
	lh->totalbytesread += bytesRead;

	fileRead = ReadFile(WVFfile, &numberOfLocations, 4, &bytesRead, NULL);
	if (!fileRead || (numberOfLocations > locLimit) || (bytesRead != 4) || (numberOfLocations * sizeof(WVFormat) != lh->filesize - 8)) {
		printf("Location number likely incorrect.\n");
		return 1;
	}
	lh->totalbytesread += bytesRead;

	constexpr unsigned int entriesToRead = 1024;
	WVFormat entry[entriesToRead];
	Location newLocation;

	lh->locations.reserve(numberOfLocations);

	DWORD bytesStillToRead = lh->filesize - 8;

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
				lh->totalbytesread += bytesRead;
			}
		}
		else
		{
			printf("Error reading entries.");
			return 2;
		}
	}

	return 0;
}