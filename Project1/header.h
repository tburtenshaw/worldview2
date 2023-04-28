#pragma once

#include "nswe.h"
#include <string>
#include <vector>
#include <imgui.h>
#include "input.h"
#include "processlocations.h"
#include "statistics.h"
#include "loadjson.h"

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

class FileLoader;
class Statistics;

struct vec2f {
	float x, y;

	operator ImVec2() const { return ImVec2(x, y); }
};

struct vec4f {
	float x, y, z, w;
};

struct UVpair {
	vec2f uv0;
	vec2f uv1;
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

	bool operator==(const RectDimension& other) const {
		return (width == other.width && height == other.height);
	}

	bool operator!=(const RectDimension& other) const {
		return !(*this == other);
	}
	
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
	
	//RGBA(float _r, float _g, float _b, float _a);
	//RGBA(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a);
	RGBA() : r(0), g(0), b(0), a(255) {}

	// constructor that accepts unsigned char values
	RGBA(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255)
		: r(_r), g(_g), b(_b), a(_a) {}

	//ints
	RGBA(int _r, int _g, int _b, int _a =255) :
		r(static_cast<unsigned char>(_r)),
		g(static_cast<unsigned char>(_g)),
		b(static_cast<unsigned char>(_b)),
		a(static_cast<unsigned char>(_a)) {}

	// constructor that accepts float values
	RGBA(float _r, float _g, float _b, float _a = 1.0f)
		: r(static_cast<unsigned char>(_r * 255.0f)),
		g(static_cast<unsigned char>(_g * 255.0f)),
		b(static_cast<unsigned char>(_b * 255.0f)),
		a(static_cast<unsigned char>(_a * 255.0f)) {}

	// doubles
	RGBA(double _r, double _g, double _b, double _a = 1.0)
		: r(static_cast<unsigned char>(_r * 255.0)),
		g(static_cast<unsigned char>(_g * 255.0)),
		b(static_cast<unsigned char>(_b * 255.0)),
		a(static_cast<unsigned char>(_a * 255.0)) {}


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

	static RGBA Lerp(const RGBA& c1, const RGBA& c2, float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return {
			static_cast<unsigned char>(c1.r + t * (c2.r - c1.r)),
			static_cast<unsigned char>(c1.g + t * (c2.g - c1.g)),
			static_cast<unsigned char>(c1.b + t * (c2.b - c1.b)),
			static_cast<unsigned char>(c1.a + t * (c2.a - c1.a))
		};
	}

	static float EuclideanRGBADistanceSquared(const RGBA& c1, const RGBA& c2) {
		return (static_cast<float>(c1.r) - static_cast<float>(c2.r)) / 255.0f * (static_cast<float>(c1.r) - static_cast<float>(c2.r)) / 255.0f
			+ (static_cast<float>(c1.g) - static_cast<float>(c2.g)) / 255.0f * (static_cast<float>(c1.g) - static_cast<float>(c2.g)) / 255.0f
			+ (static_cast<float>(c1.b) - static_cast<float>(c2.b)) / 255.0f * (static_cast<float>(c1.b) - static_cast<float>(c2.b)) / 255.0f
			+ (static_cast<float>(c1.a) - static_cast<float>(c2.a)) / 255.0f * (static_cast<float>(c1.a) - static_cast<float>(c2.a)) / 255.0f;
	}

};

struct Location {	//all the detail we can get. We process this to a leaner version called "PathPlotLocation"
	unsigned long timestamp; //we'll use a long instead of the high precision of google (seconds rather than ms)
	double longitude;	//tried using a float rather than a double means an imprecision of less than 2metres, but keeping doubles
	double latitude;		//longitude first as it's x

	int accuracy;
	int altitude;
	int heading;
	int velocity;
	int verticalaccuracy;

	int hierarchy;	//what level of the JSON it was found in

	
	std::string source;
	std::string deviceTag;
	std::string platformType;
	std::string type;
	int frequencyMhz;
	unsigned long long mac;

	unsigned long correctedTimestamp;	//corrected for daily savings/local

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

	float DistanceSquaredFrom(const PathPlotLocation& other);
	float DistanceSquaredFrom(const Location& other);
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
	bool initialised;
	FileLoader fileLoader;
	Statistics stats;
public:
	std::vector<Location> locations;	//this holds the raw data from the json file, double precision

	LODInfo lodInfo;
	void SetInitialised(bool tf);
	bool IsInitialised() const;
	bool IsLoadingFile() const;
	bool ShouldLoadFile() const;
	bool LoadedNotInitialised() const;
	bool IsFullyLoaded() const;

	std::string GetFilename() const;
	unsigned long GetNumberOfLocations() const;
	unsigned long GetFileSize() const;
	unsigned long GetFileSizeMB() const;
	unsigned long GetTotalBytesRead() const;
	float GetSecondsToLoad() const;


	int OpenAndReadLocationFile(std::wstring filename);
	int EmptyLocationInfo();
	void GenerateLocationLODs();
	void OptimiseForPaths();
	

	LocationHistory();
	~LocationHistory();

	NSWE FindBestView();
	const Statistics& GetStatistics() const;
};


struct WVFormat {
	unsigned long timestamp;
	float lon;
	float lat;
};

void size_callback(GLFWwindow* window, int windowNewWidth, int windowNewHeight);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

int StartGLProgram();
void DisplayIfGLError(const char* message, bool alwaysshow);


void ReloadBackgroundImage();
int GetBackgroundImageTexture();

int SaveWVFormat(LocationHistory& lh, std::wstring);


