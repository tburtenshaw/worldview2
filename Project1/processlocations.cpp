#include <array>
#include "header.h"
#include "processlocations.h"
#include "mytimezone.h"
#include <algorithm>

void SortAndCalculateEarliestAndLatest(LocationHistory* lh)
{
	std::sort(lh->locations.begin(), lh->locations.end());	//stable_sort might be better, as doesn't muck around if the same size
	lh->earliesttimestamp = lh->locations.front().timestamp;
	lh->latesttimestamp = lh->locations.back().timestamp;
	
	return;
	/*
	int locationssize = lh->locations.size();
	
	if (locationssize < 1)	return;

	lh->earliesttimestamp = 2147400000;	//set these to be easily beaten.
	lh->latesttimestamp = 0;

	for (int i = 0; i < locationssize; i++) {
		if (lh->locations[i].timestamp < lh->earliesttimestamp)
			lh->earliesttimestamp = lh->locations[i].timestamp;

		if (lh->locations[i].timestamp > lh->latesttimestamp)
			lh->latesttimestamp = lh->locations[i].timestamp;
	}
	*/
}

bool FurtherThan(PathPlotLocation* p1, PathPlotLocation* p2, float d) {
	if (abs(p1->latitude - p2->latitude) > d)	return true;
	if (abs(p1->longitude - p2->longitude) > d)	return true;

	return false;
}

void OptimiseDetail(std::vector<PathPlotLocation>& loc) {
	constexpr int numberOfDetailLevels = 80;

	std::array<PathPlotLocation, numberOfDetailLevels> detail;
	//std::array<int, numberOfDetailLevels> count; //count is only used for my info when deciding best sizes.

	detail[0].longitude = -500.0f;
	detail[0].latitude = -500.0f;
	for (int i = 1; i < numberOfDetailLevels; i++) {
		detail[i] = detail[0];
	}

	const float power = 0.8f;

	//printf("\nMax size: %zi, size: %zi", loc.max_size(), loc.size());

	for (std::vector<PathPlotLocation>::iterator iter = loc.begin(); iter != loc.end(); ++iter) {
		float distanceToTest = 2.0;
		bool found = false;

		if (iter->detaillevel < -1.0) {
			iter->detaillevel = -1000.0;	//if we don't want to display it ever (i.e. moving across the map for the roundtheworlds)
		}
		else
		{
			for (int i = 0; ((i < numberOfDetailLevels) && (!found)); i++) {	//go through all detail levels
				//printf("Testing %i %f.", i,distanceToTest);
				if (FurtherThan(&detail[i], iter._Ptr, distanceToTest)) {
					//printf(" %f.", distanceToTest);

					for (int e = i; e < numberOfDetailLevels; e++) {	//once we've found a point, this is propagated along all the other high detail levels
						detail[i].longitude = iter->longitude;
						detail[i].latitude = iter->latitude;

						//detail[e] = detail[i];
					}
					iter->detaillevel = distanceToTest / power;//we'll move up
					//count[i]++;
					found = true;
				}
				distanceToTest *= power;	//reduce the distance for the detail level check
			}
			if (!found) {	//set the lowest level
				iter->detaillevel = 0;// distanceToTest / power;
			}
		}

		//printf("\n%f, %f. %f", iter->longitude, iter->latitude, iter->detaillevel);
	}

	return;
}

void CreatePathPlotLocations(LocationHistory* lh)
{
	//Creates and fills a new array for GL, with floats instead of doubles and local time fixed.
	lh->pathPlotLocations.reserve(lh->locations.size());	//it'll be as big as the number of locations

	for (std::vector<Location>::iterator iter = lh->locations.begin(); iter != lh->locations.end(); ++iter) {
		lh->pathPlotLocations.emplace_back((float)iter->latitude, (float)iter->longitude, MyTimeZone::FixToLocalTime(iter->timestamp));
	}

	OptimiseDetail(lh->pathPlotLocations);
	//ColourPathPlot(lh);

	return;
}

void ColourPathPlot(LocationHistory* lh)
{
	int i = 0;

	//function pointer (probably a better way to do this in C++, but will probably move all this to shader
	RGBA(*ColourBy) (unsigned long, LocationHistory*);
	ColourBy = ColourByDayOfWeek;
	
	
	if (lh->globalOptions.colourby == 1) {
		ColourBy = ColourByHourOfDay;
	}
	for (std::vector <PathPlotLocation> ::iterator it = lh->pathPlotLocations.begin(); it != lh->pathPlotLocations.end(); ++it) {
		it->rgba = ColourBy(it->timestamp, lh);
		//it->rgba = ColourByDayOfWeek(it->timestamp, lh);
	}

	lh->globalOptions.regenPathColours = false;

	return;
}

RGBA ColourByDayOfWeek(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;

	unsigned long dayofweek = (MyTimeZone::FixToLocalTime(ts) / 86400 + 4) % 7;

	colour = lh->globalOptions.paletteDayOfWeek[dayofweek];

	return colour;
}

RGBA ColourByHourOfDay(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;

	unsigned long fixedTS = MyTimeZone::FixToLocalTime(ts);

	unsigned long secondsthroughday = fixedTS % (3600 * 24);
	int hour = 24 * secondsthroughday / (3600 * 24);

	const unsigned char rgbHour[72] = {
	0x08, 0x0F, 0x1D, 0x0E, 0x15, 0x32, 0x1A, 0x27, 0x5A, 0x25, 0x3C, 0x7F,
	0x33, 0x58, 0x9B, 0x47, 0x79, 0xB0, 0x60, 0x99, 0xC4, 0x80, 0xB8, 0xD5,
	0xA2, 0xD2, 0xE4, 0xC4, 0xE5, 0xED, 0xE0, 0xF3, 0xE5, 0xF4, 0xF7, 0xCA,
	0xFC, 0xED, 0xA5, 0xFC, 0xD6, 0x87, 0xFA, 0xB7, 0x6D, 0xF5, 0x93, 0x57,
	0xEE, 0x6D, 0x43, 0xE0, 0x4A, 0x34, 0xCA, 0x2B, 0x2A, 0xB0, 0x12, 0x25,
	0x96, 0x04, 0x22, 0x73, 0x00, 0x1B, 0x44, 0x00, 0x10, 0x1A, 0x07, 0x13
	};


	//printf("%i ", hour);
	colour.a = 255;
	colour.r = rgbHour[hour * 3 + 0];
	colour.g = rgbHour[hour * 3 + 1];
	colour.b = rgbHour[hour * 3 + 2];

	return colour;
}