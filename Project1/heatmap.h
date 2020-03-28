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

class BackgroundInfo {
public:
	BackgroundInfo();
	~BackgroundInfo();

	unsigned int vao;
	unsigned int vbo;

	Shader* shader;
	unsigned int worldTexture;	//the background NASA map
	unsigned int heatmapTexture;

	unsigned int worldTextureLocation;	//the location of this uniform
	unsigned int heatmapTextureLocation;

};