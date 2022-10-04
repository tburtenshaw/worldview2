#pragma once
#include <vector>
#include "header.h"
#include "shaders.h"
#include "heatmap.h"
#include "highresmanager.h"

//forward decls
class RGBA;
//class Shader;

struct TimeLookup {
	unsigned long t;
	size_t index;
};

class GLRenderLayer {
private:
	static unsigned int vboLocations;	//VBO that is the location data, so we don't use it multiple times
	static constexpr int lookupPieces = 32;
	static TimeLookup timeLookup[lookupPieces];	//cut up into 32 pieces, so can reduce draw data sent if not using whole time
	static void CreateTimeLookupTable(std::vector<PathPlotLocation>& locs);
	static TimeLookup knownStart;	//store these, so only does loop if it's different
	static TimeLookup knownEnd;

protected:
	unsigned int vao;	//Vertex array object
	unsigned int vbo;	//Vertex buffer object - ?maybe could share these
	void UseLocationVBO();
	
	static void LookupFirstAndCount(unsigned long starttime, unsigned long endtime, GLint *first, GLsizei *count);
	static void *locationsFront;
	static size_t locationsCount;

public:
	Shader shader;
	void SetupSquareVertices();

	GLRenderLayer()
		:vao(0), vbo(0),shader(){
	}

	//virtual void Draw() = 0;
	//void SetupShaders();
	static void CreateLocationVBO(std::vector<PathPlotLocation>& locs);
};

class BackgroundLayer : public GLRenderLayer {
private:
	void LoadBackgroundImageToTexture();
	void MakeHighresImageTexture();
public:
	unsigned int worldTexture;	//the background NASA map
	unsigned int highresTexture; //used for the higher res insert
	unsigned int NEWheatmapTexture;

	unsigned int worldTextureLocation;	//the location of this uniform
	unsigned int highresTextureLocation;
	unsigned int heatmapTextureLocation;
	
	HighResManager highres;


	void SetupShaders();
	void SetupTextures();
	void Draw(RectDimension window, const NSWE& viewNSWE, const GlobalOptions& options);

	BackgroundLayer()
		:worldTexture(0),highresTexture(0), NEWheatmapTexture(0), worldTextureLocation(0), highresTextureLocation(0), heatmapTextureLocation(0) {}

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
public:
	void SetupShaders();
	void SetupVertices();
	void BindBuffer(); //this just temporary most likely, as we should bind only in the layer code
	void Draw(float width, float height, NSWE* nswe, float linewidth, float seconds, float cycleseconds);
};

class PointsLayer : public GLRenderLayer {
private:
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
public:
	float palette[24][4];
	void SetupShaders();
	void SetupVertices();
	void Draw(float width, float height, NSWE* nswe, GlobalOptions* options);
	
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
	void Setup(int width, int height);
	void SetupVertices();
	void Draw(float width, float height, NSWE* nswe, GlobalOptions* options);
	void UpdateSize(int width, int height);

	unsigned int texture;
private:
	void SetupShaders();
	unsigned int fbo;

	//shader uniforms
	unsigned int uniformNswe;
	unsigned int uniformResolution;
	unsigned int uniformEarliestTimeToShow;
	unsigned int uniformLatestTimeToShow;
	unsigned int uniformMinimumAccuracy;
};