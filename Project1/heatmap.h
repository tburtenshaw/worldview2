#pragma once
class NSWE;
//#include "header.h"
//#include "nswe.h"


class Heatmap {
public:
	int width;
	int height;
	NSWE *nswe;

	float pixel[4096*4096];
	float maxPixel;

	Heatmap();
	~Heatmap();
};