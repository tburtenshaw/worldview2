#include "header.h"
#include "heatmap.h"
#include "nswe.h"
#include "regions.h"
#include "input.h"
#include "highresmanager.h"
#include <stdio.h>
#include <iostream>
#include <numeric>

void WorldCoord::SetFromWindowXY(float x, float y, NSWE nswe, RectDimension window)
{
	longitude = x / window.width;
	longitude *= nswe.width();
	longitude += nswe.west;

	latitude = nswe.north - y / window.height * nswe.height();

	return;
}

LocationHistory::LocationHistory()
{
	filesize = 0;

	//filename = L"d:\\Location History.json";
	filename = L"test.wvf";

	
	isFileChosen = false;
	isLoadingFile = false;
	isFullyLoaded = false;
	isInitialised = false;
	totalbytesread = 0;

}

LocationHistory::~LocationHistory()
{
	
}


PathPlotLocation::PathPlotLocation()
	:longitude(0.0f), latitude(0.0f), timestamp(0), detaillevel(0.0f),accuracy(0)
{

}

PathPlotLocation::PathPlotLocation(float lat_, float lon_, unsigned long ts_) : latitude(lat_), longitude(lon_), timestamp(ts_), detaillevel(0.0f), accuracy(0)
{

}

PathPlotLocation::PathPlotLocation(float lat_, float lon_, unsigned long ts_, int accuracy_) : latitude(lat_), longitude(lon_), timestamp(ts_), detaillevel(0.0f),accuracy(accuracy_)
{

	return (this->latitude-other.latitude)* (this->latitude - other.latitude) + (this->longitude - other.latitude)
}



//should be moved to a better cpp file

NSWE LocationHistory::FindBestView() {
	//finds the best 10 degree block, then adjusts

	//small heatmap of points
	int worlddegree[36][18] = { 0 };

	for (auto &loc : locations) {
		int xindex = (int)((loc.longitude + 180.0f)/10.0f) % 36;
		int yindex = (int)((loc.latitude + 90.0f)/10.0f) % 18;

		worlddegree[xindex][yindex]++;	//record the number of points
	}

	//find the hottest spot on the heatmap
	int bestx = 0;
	int besty = 0;
	float bestp = 0.0f;

	for (int x = 0; x < 36; x++) {
		for (int y = 0; y < 18; y++) {
			if (worlddegree[x][y] > bestp) {
				bestp = worlddegree[x][y];
				bestx = x;
				besty = y;
			}
		}
	}

	//convert this best 10x10deg block, to a slightly bigger block centred around it
	constexpr int expandAmount = 1;
	NSWE bestNSWE = { besty*10 - 90 - expandAmount,(besty + 1)*10 - 90+ expandAmount, bestx*10 - 180- expandAmount, (bestx + 1)*10 - 180+ expandAmount };

	std::cout << "best block" << bestNSWE;
	//wipe the array as we'll use it again
	memset(worlddegree, 0, sizeof(worlddegree));

	float width = bestNSWE.width();
	float height = bestNSWE.height();
	for (auto& loc : locations) {
		int xindex = ((loc.longitude - bestNSWE.west) / width) * 36;
		int yindex = ((loc.latitude - bestNSWE.south) / height) * 18;
		if ((xindex >= 0 && xindex < 36) && (yindex >= 0 && yindex < 18)) {
			worlddegree[xindex][yindex]++;	//record the number of points
		}
	}

	//now narrow down the view
	int totalcount = 0;
	int discardedcount = 0;

	int rowcount[36] = { 0 };
	for (int row = 0; row < 36; row++) { //go through each row
		for (int col = 0; col < 18; col++) {
			rowcount[row] += worlddegree[row][col];
			totalcount += worlddegree[row][col];
		}
	}

//	printf("r0:%i ", rowcount[0]);
//	printf("ra:%i\n", std::accumulate(worlddegree[0], worlddegree[0] + 18, 0));

	int firstrow = 0;
	int lastrow = 35;

	//get rid of the near empty rows at top
	while ((rowcount[firstrow] < 1) && (firstrow <= lastrow)) {
		discardedcount += rowcount[firstrow];
		firstrow++;
	}
	//and last row
	while ((rowcount[lastrow] < 1) && (lastrow >= firstrow)) {
		discardedcount += rowcount[lastrow];
		lastrow--;
	}

	//then do similar for the columns, but we'll ignore those cut  out above
	int colcount[18] = { 0 };
	for (int row = firstrow; row <= lastrow; row++) { //go through each row
		for (int col = 0; col < 18; col++) {
			colcount[col] += worlddegree[row][col];
		}
	}

	int firstcol = 0;
	int lastcol = 17;

	while ((colcount[firstcol] < 10) && (firstcol < lastcol)) {
		discardedcount += colcount[firstcol];
		firstcol++;
	}
	while ((colcount[lastcol] < 10) && (lastcol > firstcol)) {
		discardedcount += colcount[lastcol];
		lastcol--;
	}

	//printf("firstrow: %i, lastrow: %i.\n", firstrow, lastrow);
	//printf("firstcol: %i, lastcol: %i.\n", firstcol, lastcol);


	//print the matrix for debugging
	//for (int y = firstcol; y <= lastcol; y++) {
//		printf("%i: ", y);
		//for (int x = firstrow; x < lastrow; x++) {
//			printf("%04x ", worlddegree[x][y]);
		//}
		//printf("\n");
	//}









	//now should have trimmed a bit, cut off bottom or top, then left or right
	while ((discardedcount < 0.1f * totalcount) && ((firstcol < lastcol) || ((firstrow < lastrow)))) {

		//find the less detailed one of top vs bottom, and trim that row

		if (firstrow != lastrow) {
			int firstrowtotal = std::accumulate(worlddegree[firstrow] + firstcol, worlddegree[firstrow] + lastcol + 1, 0);
			int lastrowtotal = std::accumulate(worlddegree[lastrow] + firstcol, worlddegree[lastrow] + lastcol + 1, 0);
			if (firstrowtotal < lastrowtotal) {
				discardedcount += firstrowtotal;
				firstrow++;
				//printf("moving up firstrow as %i<%i now: %i\n", firstrowtotal, lastrowtotal, firstrow);
			}
			else {
				discardedcount += lastrowtotal;
				lastrow--;
				//printf("moving up lastrow as %i>%i, now: %i\n", firstrowtotal, lastrowtotal, lastrow);
			}
		}

		if (firstcol != lastcol) {

			//then left and right column sums
			int firstcoltotal = 0;
			for (int r = firstrow; r < lastrow + 1; r++) {
				firstcoltotal += worlddegree[r][firstcol];
			}
			int lastcoltotal = 0;
			for (int r = firstrow; r < lastrow + 1; r++) {
				lastcoltotal += worlddegree[r][lastcol];
			}
			//move towards the smaller one
			if (firstcoltotal < lastcoltotal) {
				discardedcount += firstcoltotal;
				firstcol++;
				//printf("moving up firstcol as %i<%i now: %i\n", firstcoltotal, lastcoltotal, firstcol);

			}
			else {
				discardedcount += lastcoltotal;
				lastcol--;
				//printf("moving up lastcol as %i>%i now: %i\n", firstcoltotal, lastcoltotal, lastcol);
			}

		}

	}

	//printf("firstrow: %i, lastrow: %i\n", firstrow, lastrow);
	//printf("firstcol: %i, lastcol: %i\n", firstcol, lastcol);
	//printf("discarded: %i out of %i.\n", discardedcount, totalcount);
	
	//here we want long to be the row, lat/longitude the columns (as screens usually wider than tall)
//as we have a 1.5x1.5degree square, and dividing by 36 in the longitude (x), gives more granularity

//start=west -50, east 10 = width=60
//if firstrow 0, lastrow 1, west=-50 deg, east = (width/36)*lastrow+1

	//firstrow = 0; lastrow = 35;
	//firstcol = 16; lastcol = 17;

	//printf("firstrow: %i, lastrow: %i\n", firstrow, lastrow);
	//printf("firstcol: %i, lastcol: %i\n", firstcol, lastcol);


	NSWE initialNSWE = bestNSWE;

	bestNSWE.west = initialNSWE.west + (width / 36.0) * firstrow;
	bestNSWE.east = initialNSWE.west + (width / 36.0) * (lastrow+1);

	bestNSWE.north = initialNSWE.south + (height / 18.0) * firstcol;
	bestNSWE.south = initialNSWE.south + (height / 18.0) * (lastcol + 1);

	//std::cout << "after cull block" << bestNSWE;

return bestNSWE;
}

void LocationHistory::GenerateStatsOnLoad()
{
	stats.numberOfLocations = locations.size();
	stats.earliestTimestamp = locations.front().correctedTimestamp;
	stats.latestTimestamp = locations.back().correctedTimestamp;

	AccuracyHistogram();


	stats.fastestVelocity = 0;
	for (auto& loc : locations) {
		if (loc.velocity < stats.velocityBins) {
			stats.histoVelocity[loc.velocity]++;
		}
		if (loc.velocity> stats.fastestVelocity) {
			stats.fastestVelocity = loc.velocity;
		}
	}
	
	for (int i = 0; i < stats.velocityBins; i++) {
		if (stats.histoVelocity[i]) {
			//printf("%i %i (%f%%)\n", i, stats.histoVelocity[i], 100.0f * (float)stats.histoVelocity[i] / (float)stats.numberOfLocations);
		}
	}


}

void LocationHistory::AccuracyHistogram()
{

	for (auto& loc : locations) {
		stats.histoAccuracy[std::max(0, std::min(loc.accuracy / stats.accuracyBinSize, stats.accuracyBins - 1))] += 1; //the integer division is deliberate, to floor it to the bin required
	}

	for (int i = 0; i < stats.accuracyBins; i++) {
		//printf("%i %i (%f%%)\n", i * stats.accuracyBinSize, stats.histoAccuracy[i], 100.0f * (float)stats.histoAccuracy[i] / (float)stats.numberOfLocations);
	}
}

double MainViewport::DegreesPerPixel()
{
	return viewNSWE.width() / static_cast<double>(windowDimensions.width);
}
