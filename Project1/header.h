#pragma once

//#include "heatmap.h"
#include "nswe.h"
#include <string>
#include <vector>
#include <imgui.h>
#include "input.h"
#include "processlocations.h"

//#define READ_BUFFER_SIZE 1024*256
//#define MAX_JSON_STRING 1024

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
//class HighResManager;
class RGBA;
class DisplayRegion;


struct vec2f {
	float x, y;

	operator ImVec2() const { return ImVec2(x, y); }
};

struct vec4f {
	float x, y, z, w;
};


struct XY {
	double x;
	double y;

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

	operator ImVec2() const { return ImVec2((float)width, (float)height); }	//converts to float for ImGui
};

class WorldCoord {
public:
	double latitude;
	double longitude;

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

	ImVec4 AsImVec4() const {
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





class MainViewport {
public:
	RectDimension windowDimensions;
	MovingTarget viewNSWE;
	std::vector<Region*> regions;

	double DegreesPerPixel(); //in the horizontal direction
};

class LocationHistory {
private:

public:
	void GenerateStatsOnLoad();
	void AccuracyHistogram();

	std::wstring filename;
	unsigned long filesize;
	float secondsToLoad;

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
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

int StartGLProgram(LocationHistory* lh);
void DisplayIfGLError(const char* message, bool alwaysshow);


int SaveWVFormat(LocationHistory* lh, std::wstring);


