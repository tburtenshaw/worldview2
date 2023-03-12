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

void LocationHistory::OptimiseForPaths() {
	constexpr int numberOfDetailLevels = 80;

	std::array<PathPlotLocation, numberOfDetailLevels> detail;


	for (int lod = 0; lod < lodInfo.numberOfLODs; lod++) {

		//set all out of bounds
		detail[0].longitude = -500.0f;
		detail[0].latitude = -500.0f;
		for (int i = 1; i < numberOfDetailLevels; i++) {
			detail[i] = detail[0];
		}

		const float power = 0.8f;


		for (unsigned long p = lodInfo.lodStart[lod]; p < lodInfo.lodStart[lod] + lodInfo.lodLength[lod];p++) {
			float distanceToTest = 2.0;
			bool found = false;


			for (int i = 0; ((i < numberOfDetailLevels) && (!found)); i++) {	//go through all detail levels
				//printf("Testing %i %f.", i,distanceToTest);
				if (FurtherThan(&detail[i], &lodInfo.pathPlotLocations[p], distanceToTest)) {
					//printf(" %f.", distanceToTest);

					for (int e = i; e < numberOfDetailLevels; e++) {	//once we've found a point, this is propagated along all the other high detail levels
						detail[i].longitude = lodInfo.pathPlotLocations[p].longitude;
						detail[i].latitude = lodInfo.pathPlotLocations[p].latitude;

						//detail[e] = detail[i];
					}
					lodInfo.pathPlotLocations[p].detaillevel = distanceToTest / power;//we'll move up
					//count[i]++;
					found = true;
				}
				distanceToTest *= power;	//reduce the distance for the detail level check
			}
			if (!found) {	//set the lowest level
				lodInfo.pathPlotLocations[p].detaillevel = 0;
				//iter->detaillevel = 0;// distanceToTest / power;
			}


			//printf("\n%f, %f. %f", iter->longitude, iter->latitude, iter->detaillevel);
		}
	}
	return;
}


void LocationHistory::GenerateLocationLODs()
{
	//Creates and fills a new array for GL, with floats instead of doubles and local time fixed.
	lodInfo.pathPlotLocations.reserve(locations.size());	//it'll likely be as big as the number of locations

	PathPlotLocation comparisonLoc;
	float precisiontarget = 10.0f / 1000000.0f; //24 deg per 1 million pixels
	int totalnumber = 0;
	int lod = 0;
	while (lod<lodInfo.numberOfLODs) {
		int number = 0;
		for (auto& loc : locations) {
			if ((fabsf(comparisonLoc.longitude - loc.longitude) > precisiontarget) || (fabsf(comparisonLoc.latitude - loc.latitude) > precisiontarget) ||
				(loc.timestamp - comparisonLoc.timestamp > 60 * 60 * 24)) {
				comparisonLoc.latitude = loc.latitude;
				comparisonLoc.longitude = loc.longitude;
				comparisonLoc.timestamp = loc.timestamp;
				number++;

				lodInfo.pathPlotLocations.emplace_back((float)loc.latitude, (float)loc.longitude, loc.correctedTimestamp, loc.accuracy);
				//each lod just comes after
			}
			else { //if the points are similar in position and time, then use the best accuracy.
				//we need to alter the last thing in the array. currentloc.accuracy = std::min(currentloc.accuracy, loc.accuracy);
				lodInfo.pathPlotLocations.back().accuracy = std::min(lodInfo.pathPlotLocations.back().accuracy, loc.accuracy);
			}
		}
		//printf("LOD: %i. Start: %i. %f. Length: %i.\n", lod, totalnumber, precisiontarget, number);
	
		lodInfo.lodStart[lod] = totalnumber;
		lodInfo.lodLength[lod] = number;
		lodInfo.lodPrecision[lod] = precisiontarget;

		totalnumber += number;
		lod++;
		precisiontarget *= 10.0f;
	}
	//printf("Total: %i. Size of struct: %i. Total MB: %f\n", totalnumber, sizeof(PathPlotLocation), (float)(totalnumber * sizeof(PathPlotLocation)) / (2 << 20));
	

	OptimiseForPaths();
	lodInfo.CreateTimeLookupTables();

}

void LODInfo::CreateTimeLookupTables()
{
	knownStart = { 0 };
	knownEnd = { 0 };

	
	//Lookup table for start/end times
	//we divide the list into 33 (pieces+1) sections, (start and end are known), so can start DrawArray at that point
	for (size_t lod=0;lod<numberOfLODs;lod++)	{
		size_t interval = lodLength[lod] / (lookupPieces + 1);
		size_t c = lodStart[lod];

		for (int i = 0; i < lookupPieces; i++) {
			c += interval;
			timeLookup[i].index = c;
			timeLookup[i].t = pathPlotLocations[c].timestamp;

			printf("LOD %i. Lookup %i. Count: %i, ts:%i\n", lod, i, c, pathPlotLocations[c].timestamp);
		}
	}
}

void LODInfo::LookupFirstAndCount(unsigned long starttime, unsigned long endtime, int lod, GLint* first, GLsizei* count)
{
//need to make work with LODs

	//Check if we've already calculated it
	if ((starttime == knownStart.t) && (endtime == knownEnd.t)) {
		*first = knownStart.index;
		*count = knownEnd.index - knownStart.index;
		//printf("k");
		return;
	}


	*first = 0;	//not this for higher LODs.
	int i = 0;
	for (; (i < lookupPieces) && (timeLookup[i].t < starttime); i++) {	//don't run if we've exceeded start time
		*first = timeLookup[i].index;	//will only be changed if
	}
	knownStart.t = starttime;
	knownStart.index = *first;

	//after this, 'i' will be at the next lookup index
	//printf("start time:%i. i %i, s: %i. \n", starttime, i, *first);
	for (int e = i; e < 32; e++) {
		//printf("endtime %i. e:%i, t:%i.\n", endtime, e, timeLookup[e].t);
		if (timeLookup[e].t >= endtime) {
			*count = timeLookup[e].index - 1 - *first;
			knownEnd.t = endtime;
			knownEnd.index = timeLookup[e].index - 1;
			//printf("E:%i\n", e);
			return;
		}
	}

	//*count = locationsCount - *first;
	knownEnd.t = endtime;
	//knownEnd.index = locationsCount-1;


}

int LODInfo::LodFromDPP(double dpp)
{
	for (int i = 0; i < numberOfLODs-1; i++) {
		if (dpp < lodPrecision[i+1]) {
			return i;
		}
	}
	
	return numberOfLODs - 1;	//return the lowest level of detail (the last in the array)
}
