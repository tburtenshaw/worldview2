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

	virtual void SetupShaders();
	void SetupSquareVertices();
};

class BackgroundInfo : public GLRenderLayer {
public:
	unsigned int worldTexture;	//the background NASA map
	//unsigned int highresTexture;
	unsigned int heatmapTexture;

	unsigned int worldTextureLocation;	//the location of this uniform
	unsigned int highresTextureLocation;
	unsigned int heatmapTextureLocation;

	void SetupShaders();

	BackgroundInfo()
		:worldTexture(0), heatmapTexture(0), worldTextureLocation(0), highresTextureLocation(0), heatmapTextureLocation(0) {}

};

class FrameBufferObjectInfo :public GLRenderLayer {
public:
	//FrameBufferObjectInfo();
	//~FrameBufferObjectInfo();

	unsigned int fbo;
	unsigned int fboTexture;
	void BindToDrawTo();	//binds this fbo

	void SetupFrameBufferObject(int width, int height);
	void Draw(float width, float height);

	//BackgroundInfo fboBGInfo;	//so can use other functions
};

class MapPathInfo : public GLRenderLayer {
public:
	void SetupShaders();
	void SetupVertices(std::vector<PathPlotLocation>& locs);
	void Draw(std::vector<PathPlotLocation>& locs, float width, float height, NSWE* nswe, float linewidth, float seconds, float cycleseconds);
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
	void SetupShaders();
	void SetupVertices(std::vector<PathPlotLocation> &locs);
	
};

class DisplayRegionsLayer : public GLRenderLayer {
public:
	std::vector<DisplayRegion> displayRegions;

	void SetupShaders();
	void SetupVertices();
	void UpdateFromRegionsData(std::vector<Region*> &dataRegions);
	void Draw(float width, float height, NSWE *nswe);
};

class DisplayRegion {	//used for holding the opengl buffer
public:
	float west, north, east, south;
	//float c[4];
	RGBA colour{ 1,2,3,4 };


};
