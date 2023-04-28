#pragma once
#include <vector>
//#include "header.h"

struct Location;

class Statistics {	//contains less necessary data calculated either when loading LH, or later
public:
	Statistics() :numberOfLocations(0), earliestTimestamp(0), latestTimestamp(0) {}
	unsigned long GetNumberOfLocations() const;
	unsigned long GetEarliestTimestamp() const;
	unsigned long GetLatestTimestamp() const;

	void GenerateStatsOnLoad(const std::vector<Location> &locations);

private:
	unsigned long numberOfLocations;
	unsigned long earliestTimestamp;
	unsigned long latestTimestamp;

	//histogram of accuracy
	static constexpr int accuracyBinSize = 5;
	static constexpr int accuracyBins = 21; //last is 100+
	int histoAccuracy[accuracyBins] = { 0 };

	static constexpr int velocityBins = 320;
	int histoVelocity[velocityBins] = { 0 };
	int fastestVelocity = 0;
	
	
	void AccuracyHistogram(const std::vector<Location>& locations);

	//friend class LocationHistory;
};