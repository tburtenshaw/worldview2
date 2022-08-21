#pragma once
#include <vector>
#include "header.h"
#include "shaders.h"

//forward decls
class RGBA;
//class Shader;

class GLRenderLayer {
public:
	unsigned int vao;
	unsigned int vbo;
	Shader shader;

	GLRenderLayer()
		:vao(0), vbo(0),shader(){

	}

	void GenerateBackgroundSquareVertices();
};

class BackgroundInfo : public GLRenderLayer {
public:
	BackgroundInfo();

	unsigned int worldTexture;	//the background NASA map
	//unsigned int highresTexture;
	unsigned int heatmapTexture;

	unsigned int worldTextureLocation;	//the location of this uniform
	unsigned int highresTextureLocation;
	unsigned int heatmapTextureLocation;
};

class FrameBufferObjectInfo {
public:
	//FrameBufferObjectInfo();
	//~FrameBufferObjectInfo();

	unsigned int fbo;
	unsigned int fboTexture;

	BackgroundInfo fboBGInfo;	//so can use other functions
};

class MapPathInfo : public GLRenderLayer {
public:
};

class MapPointsInfo : public GLRenderLayer {
public:
	unsigned int uniformNswe;
	unsigned int uniformResolution;
	unsigned int uniformPointRadius;
	unsigned int uniformPointAlpha;
	unsigned int uniformSeconds;
	unsigned int uniformCycleSeconds;
	unsigned int uniformShowHighlights;
	unsigned int uniformSecondsBetweenHighlights;
	unsigned int uniformTravelTimeBetweenHighlights;
	unsigned int uniformEarliestTimeToShow;
	unsigned int uniformLatestTimeToShow;
	unsigned int uniformPalette;
	unsigned int uniformColourBy;

	float palette[24][4];
};

class MapRegionsInfo : public GLRenderLayer {
public:
	MapRegionsInfo();

	std::vector<DisplayRegion> displayRegions;
};

class DisplayRegion {	//used for holding the opengl buffer
public:
	float west, north, east, south;
	//float c[4];
	RGBA colour{ 1,2,3,4 };
};
