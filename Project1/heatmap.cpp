#include "header.h"
#include "nswe.h"
#include "heatmap.h"
#include <vector>
#include <stdio.h>
#include <iostream>


void LocationHistory::CreateHeatmap(NSWE * inputNswe, int n) {
	
	heatmap->nswe->setto(inputNswe);
	
	heatmap->maxPixel = 0;
	for (int y = 0; y < heatmap->height; y++) {
		for (int x = 0; x < heatmap->width; x++) {
			long p;
			p= (x+y)*0;
			//if (!(x % 5)) { p = 50; }
			if ((y == 0) || (x == 0) || (x==heatmap->width-1) || (y==heatmap->height-1)) {//make the border 0
				p = 10;
			}
	
			heatmap->pixel[y * heatmap->width + x] = p;	//row,col, that's why y,x


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
				//printf("%i %i Max %i. tsdiff: %i. (%i-%i)\n", x, y, p, tsdiff, iter->timestamp, tsold);
				heatmap->maxPixel = p;
			}
			//printf("%f %f %i %i %i %i\n", iter->longitude, iter->latitude, x, y, p, tsdiff);
		}

		tsold = iter->timestamp;
		xold = x;
		yold = y;

	}

	printf("Max %f\n", heatmap->maxPixel);

	heatmap->maxPixel = 0;

	for (int y = 0; y < heatmap->height; y++) {
		for (int x = 0; x < heatmap->width; x++) {
			p = heatmap->pixel[y * heatmap->width + x];
			if (p > 1) {

				heatmap->pixel[y * heatmap->width + x] = log(p);
				//printf("%.2f ", heatmap->pixel[y * heatmap->width + x]);
				if (heatmap->pixel[y * heatmap->width + x] > heatmap->maxPixel) {
					heatmap->maxPixel = heatmap->pixel[y * heatmap->width + x];
				}
			}
		}
		//+printf("\n");
	}
	printf("Max %f\n", heatmap->maxPixel);
	return;
}


Heatmap::Heatmap()
{
	height=width = 4096;

	//width = height = 100;

	nswe = new NSWE;
	maxPixel = 0;
}

Heatmap::~Heatmap()
{
	delete nswe;
	return;
}
