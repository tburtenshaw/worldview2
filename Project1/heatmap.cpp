#include "header.h"
#include "nswe.h"
#include "heatmap.h"
#include "shaders.h"
#include <vector>
#include <stdio.h>
#include <iostream>

extern LocationHistory* pLocationHistory;

void LocationHistory::CreateHeatmap(NSWE * inputNswe, int n) {
	
	GlobalOptions* options;
	options = pLocationHistory->globalOptions;

	heatmap->nswe->setto(inputNswe);
	
	heatmap->maxPixel = 0;
	memset(heatmap->pixel, 0, sizeof(heatmap->pixel));
		
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
		
		x = (int)(fx+0.5);
		y = (int)(fy+0.5);

		if ((x < heatmap->width) && (x >= 0) && (y < heatmap->height) && (y >= 0)) {
			
			if (iter->accuracy < options->minimumaccuracy) {
				heatmap->pixel[y * heatmap->width + x] += (tsdiff * 10);
			}
			
			p = heatmap->pixel[y * heatmap->width + x];
			//printf("%i ", iter->accuracy);

			if (p > heatmap->maxPixel) {
				heatmap->maxPixel = p;
			}

		}

		tsold = iter->timestamp;
		xold = x;
		yold = y;

	}


	//heatmap->GaussianBlur(30);
	return;

}

void Heatmap::GaussianBlur(int radius)
{
	const int PascalRad[42][42] = {	//these are actually every second
	{1,0},//0 (first two rows we don't use)
	{1,0},//1

	{2, 1, 0}, //row 2
	{6, 4, 1, 0}, //row 3
	{20, 15, 6, 1, 0}, //row 4
	{70, 56, 28, 8, 1, 0}, //row 5
	{252, 210, 120, 45, 10, 1, 0}, //row 6
	{924, 792, 495, 220, 66, 12, 1, 0}, //row 7
	{2048, 1792, 1194, 597, 217, 54, 8, 0}, //row 8 , have round this and further down to get rid of insignificant figures
	{2048, 1820, 1274, 695, 289, 89, 19, 2, 0}, //row 9
	{2048, 1843, 1340, 781, 360, 128, 34, 6, 0}, //row 10
	{2048, 1861, 1396, 859, 429, 171, 53, 12, 2, 0}, //row 11
	{2048, 1877, 1444, 928, 495, 216, 76, 21, 4, 0}, //row 12
	{2048, 1890, 1485, 990, 557, 262, 101, 32, 8, 1, 0}, //row 13
	{2048, 1901, 1521, 1045, 615, 307, 129, 45, 12, 2, 0}, //row 14
	{2048, 1911, 1553, 1096, 669, 352, 158, 60, 19, 5, 1, 0}, //row 15
	{2048, 1920, 1581, 1141, 721, 396, 188, 77, 26, 7, 1, 0}, //row 16
	{2048, 1927, 1606, 1183, 769, 439, 219, 95, 35, 11, 3, 0}, //row 17
	{2048, 1934, 1628, 1221, 814, 481, 251, 115, 46, 15, 4, 1, 0}, //row 18
	{2048, 1940, 1649, 1256, 856, 521, 282, 135, 57, 21, 6, 1, 0}, //row 19
	{2048, 1945, 1667, 1288, 896, 560, 313, 156, 69, 27, 9, 2, 0}, //row 20
	{2048, 1950, 1684, 1318, 933, 597, 344, 178, 83, 34, 12, 4, 1, 0}, //row 21
	{2048, 1954, 1699, 1345, 968, 633, 375, 201, 97, 42, 16, 5, 1, 0}, //row 22
	{2048, 1958, 1714, 1371, 1002, 668, 405, 223, 111, 50, 20, 7, 2, 0}, //row 23
	{2048, 1962, 1727, 1395, 1033, 701, 435, 246, 127, 59, 25, 9, 3, 1, 0}, //row 24
	{2048, 1966, 1739, 1417, 1062, 733, 464, 269, 143, 69, 30, 12, 4, 1, 0}, //row 25
	{2048, 1969, 1750, 1437, 1090, 763, 492, 292, 159, 79, 36, 15, 5, 1, 0}, //row 26
	{2048, 1972, 1760, 1457, 1117, 792, 520, 315, 176, 90, 42, 18, 7, 2, 0}, //row 27
	{2048, 1974, 1770, 1475, 1142, 821, 547, 338, 193, 101, 49, 22, 9, 3, 1, 0}, //row 28
	{2048, 1977, 1779, 1492, 1166, 848, 573, 360, 210, 113, 56, 26, 11, 4, 1, 0}, //row 29
	{2048, 1979, 1788, 1508, 1188, 874, 599, 382, 227, 125, 64, 30, 13, 5, 2, 0}, //row 30
	{2048, 1981, 1796, 1523, 1210, 899, 624, 404, 245, 138, 72, 35, 16, 6, 2, 0}, //row 31
	{2048, 1984, 1803, 1538, 1230, 923, 648, 426, 262, 150, 81, 40, 18, 8, 3, 1, 0}, //row 32
	{2048, 1985, 1810, 1552, 1250, 946, 672, 448, 280, 163, 89, 45, 21, 9, 4, 1, 0}, //row 33
	{2048, 1987, 1817, 1564, 1268, 968, 695, 469, 297, 177, 98, 51, 25, 11, 4, 1, 0}, //row 34
	{2048, 1989, 1823, 1577, 1286, 989, 717, 490, 315, 190, 108, 57, 28, 13, 5, 2, 0}, //row 35
	{2048, 1991, 1829, 1588, 1303, 1010, 739, 510, 332, 203, 117, 64, 32, 15, 7, 2, 1, 0}, //row 36
	{2048, 1992, 1835, 1600, 1320, 1030, 760, 530, 349, 217, 127, 70, 36, 18, 8, 3, 1, 0}, //row 37
	{2048, 1994, 1840, 1610, 1335, 1049, 780, 550, 366, 231, 137, 77, 41, 20, 9, 4, 1, 0}, //row 38
	{2048, 1995, 1845, 1620, 1350, 1067, 800, 569, 383, 244, 148, 84, 45, 23, 11, 5, 2, 0}, //row 39
	{2048, 1996, 1850, 1630, 1364, 1085, 820, 588, 400, 258, 158, 91, 50, 26, 12, 5, 2, 1, 0}, //row 40
	{2048, 1998, 1855, 1639, 1378, 1103, 839, 607, 417, 272, 168, 99, 55, 29, 14, 6, 3, 1, 0}, //row 41



	};

	float factor;
	float totalfactors;
	int coeffposition;

	int cropfactor=2;
	int ystart = height / 2 - height / (2 * cropfactor);
	int yend = height / 2 + height / (2 * cropfactor);

	int xstart = width / 2 - width / (2 * cropfactor);
	int xend = width / 2 + width / (2 * cropfactor);


	if (radius > 2) {	//no blur less than this.
		//blur horizontal
		float* secondsHoriz;

		secondsHoriz = (float*)malloc(sizeof(float) * width);
		for (int y = ystart; y < yend; y++) {
			//first copy the original line from the seconds array to a temp line
			memcpy(secondsHoriz, &pixel[y * width], width * sizeof(float));

			//then go through each pixel in the line
			for (int x = xstart; x < xend; x++) {
				//treat the actual position separately
				coeffposition = 0;
				factor = PascalRad[radius][coeffposition];

				//fprintf(stdout,"factor %i, rad %i, pos %i\n", factor,radius,coeffposition);

				pixel[x + y * width] = factor * secondsHoriz[x];
				totalfactors = factor;

				while (PascalRad[radius][coeffposition]) {//move out laterally left and rightwards
					coeffposition++;
					factor = PascalRad[radius][coeffposition];

					if (x >= coeffposition) {	//don't do it if the pixel offset is lower than zero (this is the same as x-coeffpos>0);
						pixel[x + y * width] += secondsHoriz[x - coeffposition] * factor;
						totalfactors += factor;
					}

					if (x + coeffposition < width) {
						pixel[x + y * width] += secondsHoriz[x + coeffposition] * factor;
						totalfactors += factor;
					}
				}

				pixel[x + y * width] /=totalfactors;

			}

		}
		free(secondsHoriz);


		//Blur vertically
		float* secondsVert;
		secondsVert = (float*)malloc(sizeof(float) * height);
		for (int x = xstart; x < xend; x++) {
			//copying the line is more difficult vertically.
			for (int y = 0; y < yend; y++) {
				secondsVert[y] = pixel[x + y * width];
				//memcpy(&secondsVert[y], &pixel[x + y * width], sizeof(float));
			}



			for (int y =ystart; y < yend; y++) {
				//treat the actual position separately
				coeffposition = 0;
				factor = PascalRad[radius][coeffposition];

				//fprintf(stdout,"factor %i, rad %i, pos %i\n", factor,radius,coeffposition);

				pixel[x + y * width] = factor * secondsVert[y];
				totalfactors = factor;

				while (PascalRad[radius][coeffposition]) {//move out laterally left and rightwards
					coeffposition++;
					factor = PascalRad[radius][coeffposition];

					if (y >= coeffposition) {	//don't do it if the pixel offset is lower than zero
						pixel[x + y * width] += secondsVert[y - coeffposition] * factor;
						totalfactors += factor;
					}

					if (y + coeffposition < height) {
						pixel[x + y * width] += secondsVert[y + coeffposition] * factor;
						totalfactors += factor;
					}
				}

				//divide at the end
				pixel[x + y * width] /= totalfactors;
			}


		}
		free(secondsVert);
	}

	fprintf(stdout, "Gaussian blur finished.\n");

}

Heatmap::Heatmap()
{
	height=width = 2048;
	memset(pixel, 0, sizeof(pixel));

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


BackgroundInfo::BackgroundInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;

	worldTexture = 0;
	heatmapTexture = 0;

	worldTextureLocation = 0;
	heatmapTextureLocation = 0;
}

BackgroundInfo::~BackgroundInfo()
{
	delete shader;
}

MapPathInfo::MapPathInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPathInfo::~MapPathInfo()
{
	delete shader;
}

MapPointsInfo::MapPointsInfo()
{
	vao = 0;
	vbo = 0;

	shader = new Shader;
}
MapPointsInfo::~MapPointsInfo()
{
	delete shader;
}
