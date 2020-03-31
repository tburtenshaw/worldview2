#include "header.h"
#include "nswe.h"
#include "heatmap.h"
#include "shaders.h"
#include <vector>
#include <stdio.h>
#include <iostream>



void LocationHistory::CreateHeatmap(NSWE * inputNswe, int n) {
	
	heatmap->nswe->setto(inputNswe);
	
	heatmap->maxPixel = 0;
	for (int y = 0; y < heatmap->height; y++) {
		for (int x = 0; x < heatmap->width; x++) {
			heatmap->pixel[y * heatmap->width + x] = 0;	//row,col, that's why y,x
		}
	}
	
	
	unsigned long tsold;
	long tsdiff;

	tsold = locations.front().timestamp;

	float p;
	p = 0;
	
	for (std::vector<LOCATION>::iterator iter = locations.begin(); iter != locations.end(); ++iter) {
		int x,y, xold, yold;
		float fx, fy;

		tsdiff = iter->timestamp - tsold;
		if (tsdiff > 60*60*24*365) { tsdiff = 0; }
		if (tsdiff < 0) { tsdiff = 0; }

		fx = (iter->longitude - heatmap->nswe->west) / heatmap->nswe->width() * heatmap->width;
		fy = (heatmap->nswe->north - iter->latitude) / heatmap->nswe->height() * heatmap->height;
		
		x = (int)fx;
		y = (int)fy;

		if ((x < heatmap->width) && (x >= 0) && (y < heatmap->height) && (y >= 0)) {
			heatmap->pixel[y * heatmap->width + x] += (tsdiff);
			p = heatmap->pixel[y * heatmap->width + x];

			if (p > heatmap->maxPixel) {
				heatmap->maxPixel = p;
			}

		}

		tsold = iter->timestamp;
		xold = x;
		yold = y;

	}

	return;

}


Heatmap::Heatmap()
{
	height=width = 2048;
	

	//width = height = 100;

	nswe = new NSWE;
	maxPixel = 0;
	activeheatmap = 0;
}

Heatmap::~Heatmap()
{
	delete nswe;
	return;
}

