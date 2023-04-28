#include <array>
#include "header.h"
#include "processlocations.h"
#include "mytimezone.h"
#include <algorithm>
#include <chrono>
#include <iostream>

bool FurtherThan(const PathPlotLocation& p1, const PathPlotLocation& p2, const float d) {
	return (p1.latitude-p2.latitude)* (p1.latitude - p2.latitude) + (p1.longitude - p2.longitude)* (p1.longitude - p2.longitude)>d*d;
}

void LocationHistory::OptimiseForPaths() {
	constexpr int numberOfDetailLevels = 80;

	std::array<PathPlotLocation, numberOfDetailLevels> detail;


	for (int lod = 0; lod < lodInfo.GetNumberOfLODs(); lod++) {

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


			for (int detailLevel = 0; ((detailLevel < numberOfDetailLevels) && (!found)); detailLevel++) {	//go through all detail levels
				//printf("Testing %i %f.", i,distanceToTest);
				if (FurtherThan(detail[detailLevel], lodInfo.pathPlotLocations[p], distanceToTest)) {
					//printf(" %f.", distanceToTest);

					for (int propagateDown = detailLevel; propagateDown < numberOfDetailLevels; propagateDown++) {	//once we've found a point, this is propagated along all the other high detail levels
						detail[propagateDown].longitude = lodInfo.pathPlotLocations[p].longitude;
						detail[propagateDown].latitude = lodInfo.pathPlotLocations[p].latitude;
					}
					lodInfo.pathPlotLocations[p].detaillevel = distanceToTest / power;//we'll move up

					found = true;
				}
				distanceToTest *= power;	//reduce the distance for the detail level check
			}
			if (!found) {	//set the lowest level
				lodInfo.pathPlotLocations[p].detaillevel = 0;
			}

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
	while (lod<lodInfo.GetNumberOfLODs()) {
		int number = 0;
		for (auto& loc : locations) {

			//If the timestamp is the same as the previous one, we need to decide what to keep.
			if (loc.timestamp == comparisonLoc.timestamp) {
				
				//if (comparisonLoc.DistanceSquaredFrom(loc) > 0.0f) {
//					printf("TS:%i. H:%i Accuracy: %i,%i. Distance:%f\n", comparisonLoc.timestamp, loc.hierarchy, comparisonLoc.accuracy, loc.accuracy, sqrt(comparisonLoc.DistanceSquaredFrom(loc)));
	//			}

				if (comparisonLoc.accuracy>loc.accuracy) {	//the new one is more accurate, overwrite the previous
					lodInfo.pathPlotLocations.back().accuracy = loc.accuracy;
					lodInfo.pathPlotLocations.back().latitude = (float)loc.latitude;
					lodInfo.pathPlotLocations.back().longitude = (float)loc.longitude;
				}


			}
			else if ((fabsf(comparisonLoc.longitude - loc.longitude) > precisiontarget) || (fabsf(comparisonLoc.latitude - loc.latitude) > precisiontarget) ||
				(loc.timestamp - comparisonLoc.timestamp > 60 * 60 * 24)) {
				comparisonLoc.latitude = loc.latitude;
				comparisonLoc.longitude = loc.longitude;
				comparisonLoc.timestamp = loc.timestamp;
				comparisonLoc.accuracy = loc.accuracy;
				number++;

				lodInfo.pathPlotLocations.emplace_back((float)loc.latitude, (float)loc.longitude, loc.correctedTimestamp, loc.accuracy);
				//each lod just comes after the next

				//TODO: need to not have the same timepoint in these.
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

constexpr int LODInfo::GetNumberOfLODs()
{
	return numberOfLODs;
}

void LODInfo::CreateTimeLookupTables()
{
	knownStart[0] = {0,0};
	knownEnd[0] = {0};

	
	//Lookup table for start/end times
	//we divide the list into 33 (pieces+1) sections, (start and end are known), so can start DrawArray at that point
	for (size_t lod=0;lod<numberOfLODs;lod++)	{
		printf("LodStart[lod]=%i, length %i\n", lodStart[lod], lodLength[lod]);
		size_t interval = lodLength[lod] / (lookupPieces + 1);
		size_t c = lodStart[lod];

		for (int i = 0; i < lookupPieces; i++) {
			c += interval;
			timeLookup[lod][i].index = c;
			timeLookup[lod][i].t = pathPlotLocations[c].timestamp;

			//printf("LOD [%i]. Lookup [%i]. Count: %i, ts:%i\n", lod, i, c, pathPlotLocations[c].timestamp);
		}
	}


}

void LODInfo::LookupFirstAndCount(const unsigned long starttime, const unsigned long endtime, const int lod, GLint* first, GLsizei* count) const
{
//need to make work with LODs

	//Check if we've already calculated it
	if ((starttime == knownStart[lod].t) && (endtime == knownEnd[lod].t)) {
		*first = knownStart[lod].index;
		*count = knownEnd[lod].index - knownStart[lod].index;
		//printf("k");
		return;
	}


	//Find the further index for the time, which will be the start
	*first = lodStart[lod];	//not this for higher LODs.
	int i = 0;
	for (; (i < lookupPieces) && (timeLookup[lod][i].t < starttime); i++) {	//don't run if we've exceeded start time
		*first = timeLookup[lod][i].index;	//will only be changed if
	}
	knownStart[lod].t = starttime;
	knownStart[lod].index = *first;

	//after this, 'i' will be at the next lookup index
	//printf("start time:%i. i %i, s: %i. \n", starttime, i, *first);
	for (int e = i; e < 32; e++) {
		//printf("endtime %i. e:%i, t:%i.\n", endtime, e, timeLookup[e].t);
		if (timeLookup[lod][e].t >= endtime) {
			*count = timeLookup[lod][e].index - 1 - *first;
			knownEnd[lod].t = endtime;
			knownEnd[lod].index = timeLookup[lod][e].index - 1;
			//printf("E:%i\n", e);
			return;
		}
	}

	//If the end is actually beyond the points we check, then return the actual end
	*count = lodStart[lod] + lodLength[lod] - *first-1;	//TODO, check this -1 still appropriate
	knownEnd[lod].t = endtime;
	knownEnd[lod].index = lodStart[lod]+lodLength[lod] - 1;


}

int LODInfo::LodFromDPP(const double dpp) const
{
	for (int i = 0; i < numberOfLODs-1; i++) {
		if (dpp < lodPrecision[i+1]) {
			return i;
		}
	}
	
	return numberOfLODs - 1;	//return the lowest level of detail (the last in the array)
}
