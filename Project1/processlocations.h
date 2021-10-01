#pragma once
#include "header.h"

void CalculateEarliestAndLatest(LocationHistory* lh);
void OptimiseDetail(std::vector<Location>& loc);
void CreatePathPlotLocations(LocationHistory* lh);
bool FurtherThan(PathPlotLocation* p1, PathPlotLocation* p2, float d);
void ColourPathPlot(LocationHistory* lh);

RGBA ColourByDayOfWeek(unsigned long ts, LocationHistory* lh);
RGBA ColourByHourOfDay(unsigned long ts, LocationHistory* lh);
