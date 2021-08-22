#include <array>
#include "header.h"
#include "processlocations.h"
#include "mytimezone.h"

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

void CreatePathPlotLocations(LocationHistory* lh)	//this shouldn't be in the loading .cpp, as it's more a processing step
{
	//	PathPlotLocation pathPlotLoc;

	lh->pathPlotLocations.reserve(lh->locations.size());	//it'll be as big as the number of locations
	for (std::vector<LOCATION>::iterator iter = lh->locations.begin(); iter != lh->locations.end(); ++iter) {
		//		pathPlotLoc.latitude = (float)iter->latitude;
			//	pathPlotLoc.longitude = (float)iter->longitude;

				//pathPlotLoc.timestamp = iter->timestamp;

				//lh->pathPlotLocations.push_back(pathPlotLoc);
		lh->pathPlotLocations.emplace_back((float)iter->latitude, (float)iter->longitude, iter->timestamp);
	}

	OptimiseDetail(lh->pathPlotLocations);
	ColourPathPlot(lh);
	return;
}

void ColourPathPlot(LocationHistory* lh)
{
	int i = 0;
	for (std::vector <PathPlotLocation> ::iterator it = lh->pathPlotLocations.begin(); it != lh->pathPlotLocations.end(); ++it) {
		//it->rgba = ColourByHourOfDay(it->timestamp, lh);
		it->rgba = ColourByDayOfWeek(it->timestamp, lh);
	}

	lh->globalOptions->regenPathColours = false;

	return;
}

RGBA ColourByDayOfWeek(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;

	unsigned long dayofweek = (MyTimeZone::FixToLocalTime(ts) / 86400 + 4) % 7;

	colour = lh->globalOptions->paletteDayOfWeek[dayofweek];

	return colour;
}

RGBA ColourByHourOfDay(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;

	unsigned long fixedTS = MyTimeZone::FixToLocalTime(ts);

	unsigned long secondsthroughday = fixedTS % (3600 * 24);

	colour.a = 10;
	colour.r = 255;
	colour.g = (256 * secondsthroughday / (3600 * 24));
	colour.b = 0;

	return colour;
}