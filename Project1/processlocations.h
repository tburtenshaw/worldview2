#pragma once
#include "header.h"

void OptimiseDetail(std::vector<LOCATION>& loc);
void CreatePathPlotLocations(LocationHistory* lh);
bool FurtherThan(PathPlotLocation* p1, PathPlotLocation* p2, float d);
void ColourPathPlot(LocationHistory* lh);

RGBA ColourByDayOfWeek(unsigned long ts, LocationHistory* lh);