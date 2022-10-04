#include "header.h"
#include "heatmap.h"
#include "nswe.h"
#include "regions.h"
#include "input.h"
#include "highresmanager.h"
#include <stdio.h>

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
	:longitude(0.0f), latitude(0.0f), timestamp(0), detaillevel(0.0f) 
{

}

PathPlotLocation::PathPlotLocation(float lat_, float lon_, unsigned long ts_) : latitude(lat_), longitude(lon_), timestamp(ts_), detaillevel(0.0f)
{

}

PathPlotLocation::PathPlotLocation(float lat_, float lon_, unsigned long ts_, int accuracy_) : latitude(lat_), longitude(lon_), timestamp(ts_), detaillevel(0.0f),accuracy(accuracy_)
{

}

GlobalOptions::GlobalOptions()
{
	seconds = 0.0f;	//an easy way for anyone to get the seconds counter (it's set in main loop by ImGui IO)
	
	showPaths = false;
	showPoints = false;
	showHeatmap = false;

	linewidth = 2.0;
	cycleSeconds = 3600.0;

	earliestTimeToShow = 0;
	latestTimeToShow = 2147400000;

	pointdiameter = 5.0f;
	pointalpha = 0.5f;

	showHighlights = true;
	secondsbetweenhighlights=2.0f;
	minutestravelbetweenhighlights=60.0f;

	colourby = 1;

	regenPathColours = true;

	minimumaccuracy = 24;
	palette = 1;

	gaussianblur = 0.0f;
	blurperaccuracy = false;
	predictpath = false;
	heatmapmaxvalue = 500;

}

//should be moved to a better cpp file

void LocationHistory::Statistics::GenerateStatsOnLoad(const std::vector<PathPlotLocation>& locs)
{
	numberOfLocations = locs.size();
	earliestTimestamp = locs.front().timestamp;
	latestTimestamp = locs.back().timestamp;

	AccuracyHistogram(locs);
}

void LocationHistory::Statistics::AccuracyHistogram(const std::vector<PathPlotLocation>& locs)
{

	for (auto& loc : locs) {
		histoAccuracy[std::max(0, std::min(loc.accuracy / accuracyBinSize, accuracyBins - 1))] += 1; //the integer division is deliberate, to floor it to the bin required
	}

	for (int i = 0; i < accuracyBins; i++) {
		printf("%i %i (%f%%)\n", i * accuracyBinSize, histoAccuracy[i], 100.0f * (float)histoAccuracy[i] / (float)numberOfLocations);
	}
}
