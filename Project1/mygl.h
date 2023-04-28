#pragma once
#include <vector>
#include "header.h"
#include "options.h"
#include "shaders.h"
#include "heatmap.h"
#include "highresmanager.h"
#include "atlas.h"

//forward decls
class RGBA;
class MainViewport;

class GLRenderLayer {
private:
	static unsigned int vboLocations;	//VBO that is the location data, so we don't use it multiple times

protected:
	unsigned int vao;	//Vertex array object
	unsigned int vbo;	//Vertex buffer object - ?maybe could share these
	static unsigned int vaoSquare;
	static unsigned int vboSquare;

	void UseLocationVBO();
	void SetupSquareVertices();

	static void LookupFirstAndCount(unsigned long starttime, unsigned long endtime, int lod, GLint *first, GLsizei *count);
	static LocationHistory & lh;

public:
	Shader shader;


	GLRenderLayer()
		:vao(0), vbo(0),shader(){
	}

	//virtual void Draw() = 0;
	//void SetupShaders();
	static void CreateLocationVBO(LODInfo& lodInfo);
};

class BackgroundLayer : public GLRenderLayer {
private:
	
public:
	unsigned int worldTexture;	//the background NASA map
	unsigned int heatmapTexture;

	unsigned int worldTextureLocation;	//the location of this uniform
	unsigned int highresTextureLocation;
	unsigned int heatmapTextureLocation;
	
	HighResManager highres;
	Atlas atlas;

	//Uniforms
	unsigned int uniformNswe;
	unsigned int uniformDegreeSpan;
	unsigned int uniformResolution;
	unsigned int uniformMaxHeatmapValue;
	unsigned int uniformPaletteNumber;

	unsigned int uniformAtlasCount;	//number of items from the atlas we'll send in array
	unsigned int uniformAtlasNSWE;
	unsigned int uniformAtlasMult;
	unsigned int uniformAtlasAdd;

	unsigned int uniformPaletteArray;
	unsigned int uniformPaletteSize;

	void Setup();
	void SetupShaders();
	void SetupTextures();
	void Draw(MainViewport *vp);
	void LoadBackgroundImageToTexture(const char* filename);

	BackgroundLayer()
		:worldTexture(0), heatmapTexture(0), worldTextureLocation(0), highresTextureLocation(0), heatmapTextureLocation(0),
		highres(), atlas(),
		uniformNswe(0), uniformDegreeSpan(0), uniformResolution(0), uniformMaxHeatmapValue(0), uniformPaletteNumber(0),
		uniformAtlasCount(0), uniformAtlasNSWE(0), uniformAtlasMult(0), uniformAtlasAdd(0),
		uniformPaletteArray(0), uniformPaletteSize(0) {}
		

};

class FrameBufferObjectInfo :public GLRenderLayer {
private:
	unsigned int fbo;
	unsigned int fboTexture;

public:
	//FrameBufferObjectInfo();
	//~FrameBufferObjectInfo();
	void BindToDrawTo();	//binds this fbo
	void SetupFrameBufferObject(int width, int height);
	void Draw(float width, float height);
	void UpdateSize(int width, int height);
};

class PathLayer : public GLRenderLayer {
private:
	unsigned int uniformNswe;
	unsigned int uniformResolution;
	unsigned int uniformDegreeSpan;	//do calculation on CPU
	unsigned int uniformDegreeMidpoint;
	unsigned int uniformDPPHoriz;

public:
	void SetupShaders();
	void SetupVertices();
	void Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe, float linewidth, float seconds, float cycleseconds);
};

class PointsLayer : public GLRenderLayer {
private:
	unsigned int uniformNswe;
	unsigned int uniformResolution;
	unsigned int uniformDegreeSpan;	//do calculation on CPU
	unsigned int uniformDegreeMidpoint;

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
public:
	float palette[24][4];
	void SetupShaders();
	void SetupVertices();
	void Draw(LODInfo& lodInfo, int lod, float width, float height, NSWE* nswe);
	
};

class RegionsLayer : public GLRenderLayer {
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

class HeatmapLayer : public GLRenderLayer {
public:
	HeatmapLayer() : texture(0), fboToDrawHeatmap(0), fboBlur(0), blurTexture(0),
		uniformNswe(0), uniformDegreeSpan(0), uniformDegreeMidpoint(0),
		uniformResolution(0), uniformEarliestTimeToShow(0), uniformLatestTimeToShow(0),
		uniformMinimumAccuracy(0), maxvalUniformSquareSize(0), maxvalTextureLocation(0),
		maxvalTexture{ 0 } {
		readpixelBuffer = std::make_unique<float[]>(READPIXEL_BUFFER_SIZE);
	}

	void Setup(int width, int height);
	void SetupVertices();
	void Draw(const LODInfo& lodInfo, int lod, const RectDimension windowSize, const NSWE &nswe);
	void UpdateSize(int width, int height);

	unsigned int texture;	//main texture for heatmap
private:
	void SetupShaders();
	void GaussianBlur(float blurSigma, float width, float height);
	size_t FetchGaussianValues(float sigma, float* offsets, float* weights, int maxnumber);	//this in gaussian.cpp
	float FindMaxValueWithReductionShader(int width, int height, int reductionFactor);
	float ReadPixelsAndFindMax(int width, int height);

	//For determining if needs to be drawn.
	bool NeedsRedraw(const NSWE &currentNSWE, const RectDimension &windowDim);	//has anything in the Options changed that would require a redraw
	void UpdateSettingsWhenDrawn(const NSWE& currentNSWE, const RectDimension& windowDim);
	GlobalOptions optionsWhenDrawn;
	RectDimension dimensionsWhenDrawn;
	NSWE nsweWhenDrawn;


	//FBOs
	unsigned int fboToDrawHeatmap;
	unsigned int fboBlur;
	unsigned int fboMaxVal[2];
	
	//texture to blur to first
	unsigned int blurTexture;

	//blur shader and uniforms.
	Shader blurShader;
	unsigned int blurUniformPixels;
	unsigned int blurUniformGaussianValues;
	unsigned int blurUniformGaussianOffsets;

	unsigned int blurUniformResolution;
	unsigned int blurUniformDirection;
	unsigned int blurTextureLocation;

	//main shader uniforms
	unsigned int uniformNswe;
	unsigned int uniformDegreeSpan;
	unsigned int uniformDegreeMidpoint;

	unsigned int uniformResolution;
	unsigned int uniformEarliestTimeToShow;
	unsigned int uniformLatestTimeToShow;
	unsigned int uniformMinimumAccuracy;

	//max shader uniforms
	Shader maxvalShader;
	unsigned int maxvalUniformSquareSize;
	unsigned int maxvalTextureLocation;
	//texture
	unsigned int maxvalTexture[2];



	static const int READPIXEL_BUFFER_SIZE = 64 * 64;	//quite generous, I'm reducing by 4, even at 3 passes of 4k this should be enough
	std::unique_ptr<float[]> readpixelBuffer;



};