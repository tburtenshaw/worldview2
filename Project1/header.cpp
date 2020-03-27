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
