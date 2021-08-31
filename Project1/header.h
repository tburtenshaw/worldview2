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
class DisplayRegion;

//typedef struct sLocation ;

struct XY {
	float x;
	float y;

	XY operator + (const XY& a) {
		XY temp;
		temp.x = x + a.x;
		temp.y = y + a.y;
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

	void SetFromWindowXY(float x, float y, NSWE nswe, RECTDIMENSION* window);
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
	RGBA paletteHourOfDay[24]
	{
	{ 0x08, 0x0f, 0x1d, 0xff },
	{ 0x0e, 0x15, 0x32, 0xff },
	{ 0x1a, 0x27, 0x5a, 0xff },
	{ 0x25, 0x3c, 0x7f, 0xff },
	{ 0x33, 0x58, 0x9b, 0xff },
	{ 0x47, 0x79, 0xb0, 0xff },
	{ 0x60, 0x99, 0xc4, 0xff },
	{ 0x80, 0xb8, 0xd5, 0xff },
	{ 0xa2, 0xd2, 0xe4, 0xff },
	{ 0xc4, 0xe5, 0xed, 0xff },
	{ 0xe0, 0xf3, 0xe5, 0xff },
	{ 0xf4, 0xf7, 0xca, 0xff },
	{ 0xfc, 0xed, 0xa5, 0xff },
	{ 0xfc, 0xd6, 0x87, 0xff },
	{ 0xfa, 0xb7, 0x6d, 0xff },
	{ 0xf5, 0x93, 0x57, 0xff },
	{ 0xee, 0x6d, 0x43, 0xff },
	{ 0xe0, 0x4a, 0x34, 0xff },
	{ 0xca, 0x2b, 0x2a, 0xff },
	{ 0xb0, 0x12, 0x25, 0xff },
	{ 0x96, 0x04, 0x22, 0xff },
	{ 0x73, 0x00, 0x1b, 0xff },
	{ 0x44, 0x00, 0x10, 0xff },
	{ 0x1a, 0x07, 0x13, 0xff }
	};

	RGBA paletteDayOfWeek[7]
	{ {0x32,0x51,0xA7,0xff },
	  {0x96,0x54,0xa9,0xff },
	  {0xc0,0x46,0x54,0xff },
	  {0xff,0x60,0x3d,0xff },
	  {0xe4,0xb7,0x4a,0xff },
	  {0xa1,0xfc,0x58,0xff },
	  {0x00,0x82,0x94,0xff } };

	RGBA paletteMonthOfYear[12];
	bool regenPathColours;

	//points
	float pointdiameter;
	float pointalpha;
	float secondsbetweenhighlights;
	float minutestravelbetweenhighlights;

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

	MouseActions* mouseInfo;

	MovingTarget* viewNSWE;
	RECTDIMENSION* windowDimensions;

	std::vector<Region*> regions;

	FrameBufferObjectInfo* fboInfo;
	BackgroundInfo* bgInfo;
	MapPathInfo* pathInfo;
	MapPointsInfo* pointsInfo;
	MapRegionsInfo* regionsInfo;

	HighResManager* highres;

	GlobalOptions* globalOptions;
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

class DisplayRegion {	//used for holding the opengl buffer
public:
	//float west, north, east, south;
	float f[4];
	//RGBA colour;
};

class MapPointsInfo {
public:
	MapPointsInfo();
	~MapPointsInfo();

	unsigned int vao;
	unsigned int vbo;

	unsigned int uniformNswe;
	unsigned int uniformResolution;
	unsigned int uniformPointRadius;
	unsigned int uniformPointAlpha;
	unsigned int uniformSeconds;
	unsigned int uniformColourBy;
	unsigned int uniformSecondsBetweenHighlights;
	unsigned int uniformTravelTimeBetweenHighlights;
	unsigned int uniformPalette;

	float palette[24][4];

	Shader* shader;
};

class MapRegionsInfo {
public:
	MapRegionsInfo();
	~MapRegionsInfo();

	unsigned int vao;
	unsigned int vbo;
	Shader* shader;

	std::vector<DisplayRegion> displayRegions;
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
void UpdateShaderPalette(MapPointsInfo* mapPointsInfo, RGBA* sourcePalette, int n);

//regions
void SetupRegionsShaders(MapRegionsInfo* mapRegionsInfo);
void SetupRegionsBufferDataAndVertexAttribArrays(MapRegionsInfo* mapRegionsInfo);
void UpdateDisplayRegions(MapRegionsInfo* mapRegionsInfo);
void DrawRegions(MapRegionsInfo* mapRegionsInfo);