#pragma once
class NSWE;
class Shader;
//#include "header.h"
//#include "nswe.h"


class Heatmap {
public:
	int width;
	int height;
	NSWE *nswe;

	int activeheatmap;

	float pixel[4096*4096];
	float maxPixel;

	Heatmap();
	~Heatmap();
};

