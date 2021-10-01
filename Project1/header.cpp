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

	earliesttimestamp = 2145916800;
	latesttimestamp = 0;

	viewNSWE = new MovingTarget;

	heatmap = new Heatmap;

	bgInfo = new BackgroundInfo;
	pathInfo = new MapPathInfo;
	pointsInfo = new MapPointsInfo;
	regionsInfo = new MapRegionsInfo;

	highres = new HighResManager;

	globalOptions = new GlobalOptions;

	isFileChosen = false;
	isLoadingFile = false;
	isFullyLoaded = false;
	isInitialised = false;
	totalbytesread = 0;

}

LocationHistory::~LocationHistory()
{
	delete heatmap;
}

PathPlotLocation::PathPlotLocation()
	:longitude(0.0f), latitude(0.0f), timestamp(0), detaillevel(0.0f), rgba({ 0,0,0,0 })
{

}

PathPlotLocation::PathPlotLocation(float lat_, float lon_, unsigned long ts_) : latitude(lat_), longitude(lon_), timestamp(ts_), detaillevel(0.0f), rgba({0,0,0,0})
{

}

GlobalOptions::GlobalOptions()
{
	seconds = 0.0f;	//an easy way for anyone to get the seconds counter (it's set in main loop by ImGui IO)
	
	showPaths = false;
	showPoints = true;
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


	minimumaccuracy = 24;
	palette = 1;

	gaussianblur = 0.0f;
	blurperaccuracy = false;
	predictpath = false;
}