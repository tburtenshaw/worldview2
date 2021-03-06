#include <iostream>
#include <tchar.h>
#include <Windows.h>

#include "header.h"
#include "loadjson.h"
#include "heatmap.h"

int ProcessJsonBuffer(char* buffer, unsigned long buffersize, JSON_READER_STATE* jsr, vector<LOCATION>& loc, LocationHistory * lh) {

	unsigned long i;
	//char c;
#define c buffer[i]

	for (i = 0; i < buffersize; i++) {
		//c = buffer[i]; //believe it or not, this makes a small bit of difference, so I've made it a define


		//String reading
		if ((c == '\"') && (jsr->readingstring == 0)) {
			jsr->readingstring = 1;
			jsr->distancealongbuffer = 0;
		}
		else if ((c == '\"') && (jsr->readingstring == 1) && (jsr->escaped == 0)) {
			jsr->readingstring = 0;
			jsr->buffer[jsr->distancealongbuffer] = 0;
			if (jsr->readingvalue == 0) {
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
				jsr->readingvalue = 0;
			}
			jsr->distancealongbuffer = 0;

		}
		else if ((c == '\\') && jsr->escaped == 0) {
			jsr->escaped = 1;
		}
		else if (jsr->readingstring == 1) {	//if nothing special, write the character to the buffer
			jsr->buffer[jsr->distancealongbuffer] = c;
			jsr->distancealongbuffer++;
			jsr->escaped = 0;
		}

		//if we're not reading a string
		if (jsr->readingstring == 0) {

			if (c == ':') {
				jsr->readingvalue = 1;
			}
			//else if (c == ',') {
			//	jsr->readingvalue = 0;
			//}

			else if (jsr->readingnumber == 0 && jsr->readingvalue == 1)
			{
				if (((c >= '0') && (c <= '9')) || (c == '-')) {	//leading zeros aren't strictly allowed in json, but i'll accept them
					jsr->readingnumber = 1;
					jsr->distancealongbuffer = 0;
				}
			}

			if (jsr->readingnumber == 1) {
				if (((c >= '0') && (c <= '9')) || (c == '-') || (c == 'E') || (c == 'e') || (c == '+') || (c == '.')) {
					jsr->buffer[jsr->distancealongbuffer] = c;
					jsr->distancealongbuffer++;
				}
				else
				{
					jsr->readingnumber = 0;
					jsr->buffer[jsr->distancealongbuffer] = 0;
					AssignValueToName(jsr);
					jsr->distancealongbuffer = 0;
					jsr->readingvalue = 0;
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
				jsr->readingvalue = 0;
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
					jsr->locationnumber++;

					//Here we check whether the longitude spanned over 180, we make the assumption that if this occurred, we took the shortest route
					//(i.e. if we travel from NZ at longitude (174) to Vancouver (-123) we wouldn't travel through the pacific
					/*
					if ((jsr->location.longitude - jsr->oldlocation.longitude >180.0)|| (jsr->location.longitude - jsr->oldlocation.longitude < -180.0)) {	//we can't do this just yet
						BreakRoundTheWorlds(jsr, loc);
					}
					jsr->oldlocation= jsr->location;	//make the old this one
					*/

					//Write the new value into the Vector
					loc.push_back(jsr->location);

					//early/late data
					if (jsr->location.timestamp < lh->earliesttimestamp) {
						lh->earliesttimestamp = jsr->location.timestamp;
					}
					if (jsr->location.timestamp > lh->latesttimestamp) {
						lh->latesttimestamp = jsr->location.timestamp;
					}

					//reset to defaults
					jsr->location.altitude = -1;
					//jsr->location.detaillevel = 0;

					//printf("Loc:%i", jsr->locationnumber);
				}
				jsr->hierarchydepth--;

				//printf("\n");
			}

		}

	}

	return 0;
}

void BreakRoundTheWorlds(JSON_READER_STATE* jsr, vector<PathPlotLocation>& loc)
{
	int westwards = 0;
	if (jsr->location.longitude < jsr->oldlocation.longitude) {
		westwards = 1;
	}
	else {
		westwards = 0;
	}
	
	//printf("old:%f %f, new:%f %f\n", jsr->oldlocation.longitude, jsr->oldlocation.latitude, jsr->location.longitude, jsr->location.latitude);
	double newlat;
	PathPlotLocation newloc;
	double dx, dy;

	double movedlongitude;	//this is an extra 360 deg
	float proportionNew;	//proportion with the most recent
	proportionNew = 0.5;	//at the moment, just said to 50%, so the time of the new point is between the old and new

	if (!westwards) {
		movedlongitude = 360 + jsr->oldlocation.longitude;

		dx = movedlongitude - jsr->location.longitude;
		dy = jsr->oldlocation.latitude - jsr->location.latitude;

		newlat = (dy / dx) * (180 - jsr->location.longitude) + jsr->location.latitude;

		newloc.detaillevel = 0;//?don't draw
		newloc.latitude = newlat;
		newloc.longitude = -180;
		//newloc.timestamp = jsr->location.timestamp*(proportionNew)+ jsr->oldlocation.timestamp*(1-proportionNew);
		loc.push_back(newloc);

		//printf("-dx %f, dy %f. New lat: %f long %f\n", dx, dy, newloc.latitude, newloc.longitude);
		newloc.detaillevel = -1000;//?don't draw
		newloc.latitude = newlat;
		newloc.longitude = +180;
		loc.push_back(newloc);
		//printf("-dx %f, dy %f. New lat: %f long %f\n", dx, dy, newloc.latitude, newloc.longitude);
	}
	else {
		movedlongitude = 360 + jsr->location.longitude;

		//printf("old:%f %f, new:%f %f\n", jsr->oldlocation.longitude, jsr->oldlocation.latitude, movedlongitude, jsr->location.latitude);

		dx = movedlongitude - jsr->oldlocation.longitude;
		dy = jsr->location.latitude- jsr->oldlocation.latitude;

		newlat = (dy / dx) * (180 - movedlongitude) + jsr->location.latitude;

		newloc.detaillevel = 0;
		newloc.latitude = newlat;
		newloc.longitude = 180;
		//newloc.timestamp = jsr->location.timestamp * (proportionNew)+jsr->oldlocation.timestamp * (1 - proportionNew);
		loc.push_back(newloc);

		
		newloc.detaillevel = -1000.0;//?don't draw
		newloc.latitude = newlat;
		newloc.longitude = -180.0;
		loc.push_back(newloc);
		//printf("-dx %f, dy %f. New lat: %f long %f\n", dx, dy, newloc.latitude, newloc.longitude);
	}
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

int AssignValueToName(JSON_READER_STATE* jsr)
{
	//printf("%s=%s ", jsr->name, jsr->buffer);
	

	//use the long of the first four characters to speed things up
	long * firstlong = (long *)jsr->name;
	//printf("%s %x", jsr->name,firstlong[0]);


	switch (firstlong[0]) {
		
	case 0x656d6974:
		if (!strcmp(jsr->name, "timestampMs")) {
			//printf("TS %s ", jsr->buffer);
			jsr->location.timestamp = fast_strtotimestamp(jsr->buffer);
			//printf("TS %i ", jsr->location.timestamp);
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
