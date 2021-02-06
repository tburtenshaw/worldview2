#include "header.h"
#include "processlocations.h"

bool FurtherThan(PathPlotLocation* p1, PathPlotLocation* p2, float d) {
	if (abs(p1->latitude - p2->latitude) > d)	return true;
	if (abs(p1->longitude - p2->longitude) > d)	return true;

	return false;
}

void OptimiseDetail(std::vector<PathPlotLocation>& loc) {

	int i;
	PathPlotLocation detail0;
	PathPlotLocation detail1;
	PathPlotLocation detail2;
	PathPlotLocation detail3;
	PathPlotLocation detail4;
	PathPlotLocation detail5;

	detail0.latitude = -500.0f;	//these are deliberately out of bounds
	detail0.longitude = -500.0f;
	detail1 = detail2 = detail3 = detail4 = detail5 = detail0;

	const int detaillevels = 80;

	std::vector<PathPlotLocation> detail;
	std::vector<int> count;

	detail.resize(detaillevels);
	count.resize(detaillevels);

	detail[0].longitude = -500.0f;
	detail[0].latitude = -500.0f;
	for (i = 1; i < detaillevels; i++) {
		detail[i] = detail[0];
	}

	float power = 0.8f;

	//printf("\nMax size: %zi, size: %zi", loc.max_size(), loc.size());


	for (std::vector<PathPlotLocation>::iterator iter = loc.begin(); iter != loc.end(); ++iter) {
		float d = 2.0;
		int notfound = 1;

		if (iter->detaillevel < -1.0) {
			iter->detaillevel = -1000.0;	//if we don't want to display it ever (i.e. moving across the map for the roundtheworlds)
		}
		else
		{
			for (i = 0; ((i < detaillevels) && (notfound)); i++) {	//go through all detail levels
				//printf("Testing %i %f.", i,d);
				if (FurtherThan(&detail[i], iter._Ptr, d)) {
					//printf(" %f.", d);
					detail[i].longitude = iter->longitude;
					detail[i].latitude = iter->latitude;

					for (int e = i + 1; e < detaillevels; e++) {
						detail[e] = detail[i];
					}
					iter->detaillevel = d / power;//we'll move up
					count[i]++;
					notfound = 0;
				}
				d *= power;
			}
			if (notfound) {	//set the lowest level
				iter->detaillevel = 0;// d / power;
			}
		}

		//printf("\n%f, %f. %f", iter->longitude, iter->latitude, iter->detaillevel);

	}


	return;
}

void CreatePathPlotLocations(LocationHistory* lh)	//this shouldn't be in the loading .cpp, as it's more a processing step
{
	PathPlotLocation pathPlotLoc;

	for (std::vector<LOCATION>::iterator iter = lh->locations.begin(); iter != lh->locations.end(); ++iter) {

		pathPlotLoc.latitude = (float)iter->latitude;
		pathPlotLoc.longitude = (float)iter->longitude;
		pathPlotLoc.rgba.r = 255;
		pathPlotLoc.rgba.g = 255;
		pathPlotLoc.rgba.b = 100;
		pathPlotLoc.rgba.a = 255;

		pathPlotLoc.timestamp = iter->timestamp;
		pathPlotLoc.rgba = ColourByDayOfWeek(pathPlotLoc.timestamp);

		//pathPlotLoc.detaillevel = iter->detaillevel;	//eventually, the original won't need this


		lh->pathPlotLocations.push_back(pathPlotLoc);

		//printf("%f %f %f %f\t",iter->latitude, pathPlotLoc.latitude);
	}

	OptimiseDetail(lh->pathPlotLocations);

	return;
}

RGBA ColourByDayOfWeek(unsigned long ts)
{
	RGBA colour;
	//colour.a = 255;
	
	const RGBA cpd[7] = { {0x32,0x51,0xA7,0xFF},
{0xc0,0x46,0x54,0xFF},
{0xe1,0x60,0x3d,0x3F},
{0xe4,0xb7,0x4a,0x0F},
{0xa1,0xfc,0x58,0x0F},
{0x96,0x54,0xa9,0x0F},
{0x00,0x82,0x94,0x0F} };


	unsigned long dayofweek=	(ts / 86400 + 4) % 7;
	
	
	//printf("d%i ", dayofweek);

	colour = cpd[dayofweek];

	return colour;
}
