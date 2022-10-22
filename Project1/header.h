#pragma once

//#include "heatmap.h"
#include "nswe.h"
#include <string>
#include <vector>
#include <imgui.h>
#include "input.h"

#define READ_BUFFER_SIZE 1024*256
#define MAX_JSON_STRING 1024

//forward declarations
//class NSWE;
struct Location;
class Heatmap;
class FrameBufferObjectInfo;
class BackgroundLayer;
class PathLayer;
class PointsLayer;
class RegionsLayer;
class Region;
//class MovingTarget;
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

struct RectDimension {
	int width;
	int height;
};

class WorldCoord {
public:
	float latitude;	//these can be floats, as they're not really for data points
	float longitude;

	void SetFromWindowXY(float x, float y, NSWE nswe, RectDimension window);
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

	void operator=(const ImVec4 other) {
		r = (unsigned char)(other.x * 255.0f);
		g = (unsigned char)(other.y * 255.0f);
		b = (unsigned char)(other.z * 255.0f);
		a = (unsigned char)(other.w * 255.0f);
	}

	ImVec4 AsImVec4(){
		return { (float)r / 255.0f, (float)g / 255.0f ,(float)b / 255.0f ,(float)a / 255.0f };
	}

};

struct Location {
	unsigned long timestamp; //we'll use a long instead of the high precision of google (seconds rather than ms)
	double longitude;	//tried using a float rather than a double means an imprecision of less than 2metres, but keeping doubles
	double latitude;		//longitude first as it's x

	int accuracy;
	int altitude;
	int heading;
	int velocity;
	int verticalaccuracy;

	unsigned long correctedTimestamp;

	bool operator< (Location const& rhs) {	//compares (i.e. sorts) just by timestamp
		return timestamp < rhs.timestamp;
	}
};

struct PathPlotLocation {	//this is the structure (a vector of them) sent to the GPU
	float longitude;
	float latitude;

	unsigned long timestamp;

	float detaillevel;
	int accuracy;

	PathPlotLocation();
	PathPlotLocation(float lat, float lon, unsigned long ts);
	PathPlotLocation(float lat, float lon, unsigned long ts, int accuracy);
};

class GlobalOptions {
public:
	GlobalOptions();
	bool showPaths;
	bool showPoints;
	bool showHeatmap;
	float seconds;

	//
	unsigned long earliestTimeToShow;
	unsigned long latestTimeToShow;

	//paths
	float linewidth;
	float cycleSeconds;
	int colourby;

	//palettes
	int indexPaletteHour = 0;
	int indexPaletteWeekday = 0;
	int indexPaletteYear = 0;


	RGBA paletteHourOfDay[24]
	{
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
	{ 0x1a, 0x07, 0x13, 0xff },
	{ 0x08, 0x0f, 0x1d, 0xff }
	};

	RGBA paletteDayOfWeek[7]
	{ {0x32,0x51,0xA7,0xff },
	  {0x96,0x54,0xa9,0xff },
	  {0xc0,0x46,0x54,0xff },
	  {0xff,0x60,0x3d,0xff },
	  {0xe4,0xb7,0x4a,0xff },
	  {0xa1,0xfc,0x58,0xff },
	  {0x00,0x82,0x94,0xff } };

	RGBA paletteYear[24]{
{0xfd, 0xfd, 0x16, 0xff},
{0xd0, 0x4d, 0x60, 0xff},
{0xb1, 0xbe, 0xfa, 0xff},
{0x1c, 0xfd, 0xf1, 0xff},
{0xf9, 0x85, 0x00, 0xff},
{0xa9, 0x55, 0xd0, 0xff},
{0x9b, 0x6c, 0x6a, 0xff},
{0xfb, 0xc8, 0x7a, 0xff},
{0xed, 0xff, 0xed, 0xff},
{0x9f, 0xe6, 0x00, 0xff},
{0x22, 0xc7, 0xe9, 0xff},
{0xfc, 0x0d, 0xf8, 0xff},
{0xf6, 0x00, 0x00, 0xff},
{0xfe, 0x76, 0xc8, 0xff},
{0x3d, 0x84, 0x84, 0xff},
{0xc0, 0x0d, 0xfe, 0xff},
{0x00, 0xaa, 0xfd, 0xff},
{0xff, 0x00, 0x85, 0xff},
{0x6d, 0xfc, 0xb2, 0xff},
{0x49, 0x69, 0xff, 0xff},
{0xe5, 0xff, 0xa3, 0xff},
{0xff, 0xc3, 0xef, 0xff},
{0x4f, 0x8a, 0x35, 0xff},
{0x00, 0xfe, 0x5d, 0xff}
	};

	RGBA paletteMonthOfYear[12];

	//points
	float pointdiameter;
	float pointalpha;
	bool showHighlights;
	float secondsbetweenhighlights;
	float minutestravelbetweenhighlights;

	//heatmap
	int palette; //viridis = 1, inferno = 2
	int minimumaccuracy;
	bool predictpath;
	float gaussianblur;
	float heatmapmaxvalue;
	float debug;

};


class LODInfo {
public:
	static constexpr int numberOfLODs = 4;
	unsigned long lodStart[numberOfLODs];
	unsigned long lodLength[numberOfLODs];
	float lodPrecision[numberOfLODs];

	std::vector<PathPlotLocation> pathPlotLocations;	//contains multiple LODs after each other with some info removed, floats vs doubles etc.

	int LodFromDPP(float dpp);
};

class LocationHistory {
private:

public:
	void GenerateStatsOnLoad();
	void AccuracyHistogram();

	std::wstring filename;
	unsigned long filesize;

	std::vector<Location> locations;	//this holds the raw data from the json file, double precision

	LODInfo lodInfo;


	int OpenAndReadLocationFile();
	int CloseLocationFile();
	void GenerateLocationLODs();
	void OptimiseForPaths();

	LocationHistory();
	~LocationHistory();

	NSWE FindBestView();

//should be in another class re file loading
	bool isFileChosen;
	bool isFullyLoaded;
	bool isLoadingFile;
	bool isInitialised;
	unsigned long totalbytesread;

//should be in another class re: overall view
	MovingTarget viewNSWE;
	RectDimension windowDimensions;

	std::vector<Region*> regions;

	
	GlobalOptions globalOptions;

	class Statistics {	//contains less necessary data calculated either when loading LH, or later
	public:
		unsigned long numberOfLocations;
		unsigned long earliestTimestamp;
		unsigned long latestTimestamp;
		Statistics() :numberOfLocations(0), earliestTimestamp(0), latestTimestamp(0) {}

	private:
		//histogram of accuracy
		static constexpr int accuracyBinSize = 5;
		static constexpr int accuracyBins = 21; //last is 100+
		int histoAccuracy[accuracyBins] = { 0 };

		static constexpr int velocityBins = 320;
		int histoVelocity[velocityBins] = {0};
		int fastestVelocity = 0;


		friend class LocationHistory;
	} stats;

};


struct WVFormat {
	unsigned long timestamp;
	float lon;
	float lat;
};

void size_callback(GLFWwindow* window, int windowNewWidth, int windowNewHeight);
int StartGLProgram(LocationHistory* lh);
void DisplayIfGLError(const char* message, bool alwaysshow);


int SaveWVFormat(LocationHistory* lh, std::wstring);


