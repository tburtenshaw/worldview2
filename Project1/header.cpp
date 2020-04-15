#include "header.h"
#include "heatmap.h"
#include "nswe.h"
#include "regions.h"
#include "input.h"
#include "highresmanager.h"
#include <stdio.h>

void WORLDCOORD::SetFromWindowXY(float x, float y, NSWE nswe, RECTDIMENSION *window)
{
	longitude = x / window->width;
	longitude *= nswe.width();
	longitude += nswe.west;

	latitude = nswe.north - y / window->height * nswe.height();

	return;
}

LocationHistory::LocationHistory()
{
	filesize = 0;

	filename = L"d:/location history.json";

	earliesttimestamp = 2145916800;
	latesttimestamp = 0;

	windowDimensions = new RECTDIMENSION;
	mouseInfo = new MouseActions;
	viewNSWE = new MovingTarget;


	heatmap = new Heatmap;

	bgInfo = new BackgroundInfo;
	pathInfo = new MapPathInfo;
	pointsInfo = new MapPointsInfo;

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

GlobalOptions::GlobalOptions()
{
	seconds = 0;
	
	showPaths = true;
	showPoints = false;
	showHeatmap = true;

	linewidth = 2;
	cycle = 3600;
	pointradius = 20;

	minimumaccuracy = 24;
	palette = 1;

	gaussianblur = 0.0;
	blurperaccuracy = false;
	predictpath = false;
}