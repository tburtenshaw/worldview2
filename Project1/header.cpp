#include "header.h"
#include "heatmap.h"
#include "nswe.h"
#include "regions.h"
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

	earliesttimestamp = 2145916800;
	latesttimestamp = 0;

	windowDimensions = new RECTDIMENSION;
	longlatMouse = new WORLDCOORD;
	viewNSWE = new movingTarget;


	heatmap = new Heatmap;
	viewportRegion = new Region;


	bgInfo = new BackgroundInfo;
	pathInfo = new MapPathInfo;
	pointsInfo = new MapPointsInfo;

	globalOptions = new GlobalOptions;

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
	
	showPaths = false;
	showPoints = false;

	linewidth = 2;
	cycle = 3600;
	pointradius = 20;

	minimumaccuracy = 24;
	palette = 1;

	blurperaccuracy = false;
	predictpath = false;
}
