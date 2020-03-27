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
	float latitude;
	float longitude;

	void SetFromWindowXY(float x, float y, NSWE nswe, RECTDIMENSION window);

};


struct LOCATION {
	unsigned long timestamp; //we'll use a long instead of the high precision of google (seconds rather than ms)	
	float longitude;	//using a float rather than a double means an imprecision of less than 2metres
	float latitude;		//longitude first as it's x

	float detaillevel;

	int accuracy;
	int altitude;
	int heading;
	int velocity;
	int verticalaccuracy;
};


class LocationHistory {
public:
	std::string filename;
	std::vector<LOCATION> locations;
	Heatmap* heatmap;

	LocationHistory();
	~LocationHistory();
	void CreateHeatmap(NSWE *nswe, int n);
};



