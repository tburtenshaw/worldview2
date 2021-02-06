#pragma once
#include "header.h"

void OptimiseDetail(std::vector<LOCATION>& loc);
void CreatePathPlotLocations(LocationHistory* lh);
bool FurtherThan(LOCATION* p1, LOCATION* p2, float d);

RGBA ColourByDayOfWeek(unsigned long ts);