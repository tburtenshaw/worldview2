#pragma once
#include "header.h"
struct PathPlotLocation;

struct TimeLookup {
	unsigned long t;
	size_t index;
};

class LODInfo {
private:
	static constexpr int lookupPieces = 32;
	TimeLookup timeLookup[lookupPieces];	//cut up into 32 pieces, so can reduce draw data sent if not using whole time
	TimeLookup knownStart;	//store these, so only does loop if it's different
	TimeLookup knownEnd;

public:
	static constexpr int numberOfLODs = 4;
	unsigned long lodStart[numberOfLODs];
	unsigned long lodLength[numberOfLODs];
	float lodPrecision[numberOfLODs];

	std::vector<PathPlotLocation> pathPlotLocations;	//contains multiple LODs after each other with some info removed, floats vs doubles etc.

	void CreateTimeLookupTables();
	void LookupFirstAndCount(unsigned long starttime, unsigned long endtime, int lod, GLint* first, GLsizei* count);
	int LodFromDPP(double dpp);
};


bool FurtherThan(PathPlotLocation* p1, PathPlotLocation* p2, float d);
