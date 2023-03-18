#pragma once
#include "header.h"
struct PathPlotLocation;

struct TimeLookup {
	unsigned long t;
	size_t index;
};

class LODInfo {
private:
	static constexpr int numberOfLODs = 4;
	static constexpr int lookupPieces = 32;
	TimeLookup timeLookup[numberOfLODs][lookupPieces];	//cut up into 32 pieces, so can reduce draw data sent if not using whole time
	mutable TimeLookup knownStart[numberOfLODs];	//store these, so only checks through loop if it's different
	mutable TimeLookup knownEnd[numberOfLODs];

public:
	constexpr int GetNumberOfLODs();
	
	unsigned long lodStart[numberOfLODs];
	unsigned long lodLength[numberOfLODs];
	float lodPrecision[numberOfLODs];

	std::vector<PathPlotLocation> pathPlotLocations;	//contains multiple LODs after each other with some info removed, floats vs doubles etc.

	void CreateTimeLookupTables();
	void LookupFirstAndCount(const unsigned long starttime, const unsigned long endtime, const int lod, GLint* first, GLsizei* count) const;
	int LodFromDPP(const double dpp);
};



