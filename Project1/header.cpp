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

	filename = L"d:/Location History.json";

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
	
	showPaths = false;
	showPoints = false;
	showHeatmap = true;

	linewidth = 2;
	cycle = 3600;

	/*paletteDayOfWeek[7]={ {0x32, 0x51, 0xA7, 0xFF},
		{ 0xc0,0x46,0x54,0xFF },
		{ 0xe1,0x60,0x3d,0x3F },
		{ 0xe4,0xb7,0x4a,0x0F },
		{ 0xa1,0xfc,0x58,0x0F },
		{ 0x96,0x54,0xa9,0x0F },
		{ 0x00,0x82,0x94,0x0F } };
		*/


	pointradius = 20;



	minimumaccuracy = 24;
	palette = 1;

	gaussianblur = 0.0;
	blurperaccuracy = false;
	predictpath = false;
}