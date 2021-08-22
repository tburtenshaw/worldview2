#pragma once

//#include "heatmap.h"
//#include "nswe.h"
#include <string>
#include <vector>


#define READ_BUFFER_SIZE 1024*256
#define MAX_JSON_STRING 1024

//forward declarations
class NSWE;
struct LOCATION;
class Heatmap;
class Shader;
class FrameBufferObjectInfo;
class BackgroundInfo;
class MapPathInfo;
class MapPointsInfo;
class MapRegionsInfo;
class Region;
class MovingTarget;
class MouseActions;
class HighResManager;
class RGBA;

//typedef struct sLocation ;

struct XY {
	float x;
	float y;

	XY operator + (const XY& a) {
		XY temp;
		temp.x = x+a.x;
		temp.y = y+a.y;
		return temp;
	}
	XY operator - (const XY& a) {
		XY temp;
		temp.x = x - a.x;
		temp.y = y - a.y;
		return temp;
	}
};

struct RECTDIMENSION {
	int width;
	int height;
};

class WORLDCOORD {
public:
	float latitude;	//these can be floats, as they're not really for data points
	float longitude;

	void SetFromWindowXY(float x, float y, NSWE nswe, RECTDIMENSION *window);

};

class RGBA {
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	void operator=(const RGBA other) {
		r = other.r;
		g = other.g;
		b = other.b;
		a = other.a;
	}
};

struct LOCATION {
	unsigned long timestamp; //we'll use a long instead of the high precision of google (seconds rather than ms)	
	double longitude;	//tried using a float rather than a double means an imprecision of less than 2metres, but keeping doubles
	double latitude;		//longitude first as it's x

	//float detaillevel;

	int accuracy;
	int altitude;
	int heading;
	int velocity;
	int verticalaccuracy;
};

struct PathPlotLocation {	//this is the structure (a vector of them) sent to the GPU
	float longitude;
	float latitude;

	unsigned long timestamp;
	RGBA rgba;

	float detaillevel;

	PathPlotLocation();
	PathPlotLocation(float lat, float lon, unsigned long ts);
};


class GlobalOptions {
public:
	GlobalOptions();
	bool showPaths;
	bool showPoints;
	bool showHeatmap;
	float seconds;

	//paths
	float linewidth;
	float cycle;
	int colourby;
	RGBA paletteDayOfWeek[7]
	
	{ {0x32,0x51,0xA7,0xff },
	  {0xc0,0x46,0x54,0xff },
	  {0xff,0x60,0x3d,0xff },
	  {0xe4,0xb7,0x4a,0xff },
	  {0xa1,0xfc,0x58,0xff },
	  {0x96,0x54,0xa9,0xff },
	  {0x00,0x82,0x94,0xff } };
	
	RGBA paletteMonthOfYear[12];
	bool regenPathColours;


	//points
	float pointradius;


	//heatmap
	int palette; //viridis = 1, inferno = 2
	bool blurperaccuracy;
	int minimumaccuracy;
	bool predictpath;
	float gaussianblur;
};


class LocationHistory {
public:
	std::wstring filename;
	unsigned long filesize;
	
	std::vector<LOCATION> locations;	//this holds the raw data from the json file, double precision
	unsigned long earliesttimestamp;
	unsigned long latesttimestamp;
	
	std::vector<PathPlotLocation> pathPlotLocations;	//a more minimal version with floats, ready for the GPU

	Heatmap* heatmap;

	LocationHistory();
	~LocationHistory();
	
	bool isFileChosen;
	bool isFullyLoaded;
	bool isLoadingFile;
	bool isInitialised;
	unsigned long totalbytesread;

	MouseActions *mouseInfo;

	MovingTarget *viewNSWE;
	RECTDIMENSION *windowDimensions;

	std::vector<Region*> regions;

	FrameBufferObjectInfo *fboInfo;
	BackgroundInfo *bgInfo;
	MapPathInfo *pathInfo;
	MapPointsInfo *pointsInfo;
	MapRegionsInfo* regionsInfo;

	HighResManager* highres;

	GlobalOptions *globalOptions;

};

class BackgroundInfo {
public:
	BackgroundInfo();
	~BackgroundInfo();

	unsigned int vao;
	unsigned int vbo;

	Shader* shader;
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


class MapPathInfo {
public:
	MapPathInfo();
	~MapPathInfo();
	
	unsigned int vao;
	unsigned int vbo;

	Shader* shader;

};


class MapPointsInfo {
public:
	MapPointsInfo();
	~MapPointsInfo();

	unsigned int vao;
	unsigned int vbo;

	Shader* shader;

};

class MapRegionsInfo {
public:

};


int StartGLProgram(LocationHistory* lh);
void DisplayIfGLError(const char* message, bool alwaysshow);

void SetupBackgroundVertices(BackgroundInfo* backgroundInfo);
void LoadBackgroundImageToTexture(unsigned int* texture);
void MakeHighresImageTexture(unsigned int* texture);
void MakeHeatmapTexture(NSWE* nswe, unsigned int* texture);
void SetupBackgroundShaders(BackgroundInfo* backgroundInfo);
void SetupFrameBufferObject(FrameBufferObjectInfo* fboInfo, int width, int height);
void DrawFrameBuffer(LocationHistory* lh);

void UpdateHeatmapTexture(NSWE* nswe, BackgroundInfo* backgroundInfo);

void DrawBackgroundAndHeatmap(LocationHistory* lh);

//paths
void SetupPathsBufferDataAndVertexAttribArrays(MapPathInfo* mapPathInfo);
void SetupPathsShaders(MapPathInfo* mapPathInfo);
void DrawPaths(MapPathInfo* mapPathInfo);

//points
void SetupPointsBufferDataAndVertexAttribArrays(MapPointsInfo* mapPointsInfo);
void SetupPointsShaders(MapPointsInfo* mapPointsInfo);
void DrawPoints(MapPointsInfo* mapPointsInfo);


//regions
void DrawRegions(MapRegionsInfo* mapRegionsInfo);