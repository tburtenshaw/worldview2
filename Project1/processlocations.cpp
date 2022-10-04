#include <array>
#include "header.h"
#include "processlocations.h"
#include "mytimezone.h"
#include <algorithm>

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

void LocationHistory::CreatePathPlotLocations()
{
	//Creates and fills a new array for GL, with floats instead of doubles and local time fixed.
	pathPlotLocations.reserve(locations.size());	//it'll be as big as the number of locations

	for (std::vector<Location>::iterator iter = locations.begin(); iter != locations.end(); ++iter) {
		pathPlotLocations.emplace_back((float)iter->latitude, (float)iter->longitude, MyTimeZone::FixToLocalTime(iter->timestamp),iter->accuracy);
	}

	OptimiseDetail(pathPlotLocations);

	return;
}
