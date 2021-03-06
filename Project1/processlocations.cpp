#include "header.h"
#include "processlocations.h"
#include "mytimezone.h"

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


		pathPlotLoc.timestamp = iter->timestamp;

		lh->pathPlotLocations.push_back(pathPlotLoc);

		//printf("%f %f %f %f\t",iter->latitude, pathPlotLoc.latitude);
	}

	OptimiseDetail(lh->pathPlotLocations);
	ColourPathPlot(lh);
	return;
}

void ColourPathPlot(LocationHistory* lh)
{
	
	int i = 0;
	for (std::vector <PathPlotLocation> ::iterator it = lh->pathPlotLocations.begin(); it != lh->pathPlotLocations.end(); ++it) {
		it->rgba = ColourByHourOfDay(it->timestamp, lh);
	}
		
	lh->globalOptions->regenPathColours = false;
	
	return;
}

RGBA ColourByDayOfWeek(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;
		
	unsigned long dayofweek=	(MyTimeZone::FixToLocalTime(ts) / 86400 + 4) % 7;

	colour = lh->globalOptions->paletteDayOfWeek[dayofweek];

	return colour;
}

RGBA ColourByHourOfDay(unsigned long ts, LocationHistory* lh)
{
	RGBA colour;

	unsigned long fixedTS = MyTimeZone::FixToLocalTime(ts);

	unsigned long secondsthroughday = fixedTS % (3600* 24);

	colour.a = 10;
	colour.r = 255;
	colour.g = (256*secondsthroughday / (3600 * 24));
	colour.b = 0;


	return colour;
}

