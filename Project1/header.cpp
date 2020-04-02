#include "header.h"
#include "heatmap.h"
#include "nswe.h"
#include <stdio.h>

void WORLDCOORD::SetFromWindowXY(float x, float y, NSWE nswe, RECTDIMENSION window)
{
	longitude = x / window.width;
	longitude *= nswe.width();
	longitude += nswe.west;

	latitude = nswe.north - y / window.height * nswe.height();

	return;
}

LocationHistory::LocationHistory()
{
	heatmap = new Heatmap;
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
}
