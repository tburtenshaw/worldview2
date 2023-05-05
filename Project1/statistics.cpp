#include "header.h"
#include "statistics.h"
#include <unordered_map>
#include <iostream>
#include <filesystem>

unsigned long Statistics::GetNumberOfLocations() const
{
	return numberOfLocations;
}

unsigned long Statistics::GetEarliestTimestamp() const
{
	return earliestTimestamp;
}

unsigned long Statistics::GetLatestTimestamp() const
{
	return latestTimestamp;
}

int Statistics::GetAccuracyBins() const
{
	return accuracyBins;
}


void Statistics::AccuracyHistogram(const std::vector<Location>& locations)
{

	for (auto& loc : locations) {
		histoAccuracy[std::max(0, std::min(loc.accuracy / accuracyBinSize, accuracyBins - 1))] += 1; //the integer division is deliberate, to floor it to the bin required
	}

	for (int i = 0; i < accuracyBins; i++) {
		//printf("%i %i (%f%%)\n", i * stats.accuracyBinSize, stats.histoAccuracy[i], 100.0f * (float)stats.histoAccuracy[i] / (float)stats.numberOfLocations);
	}
}

void Statistics::GenerateStatsOnLoad(const std::vector<Location>& locations)
{

	numberOfLocations = locations.size();
	earliestTimestamp = locations.front().correctedTimestamp;
	latestTimestamp = locations.back().correctedTimestamp;
	

	AccuracyHistogram(locations);


	std::unordered_map<std::string, int> freqSource;
	for (auto& loc : locations) {
		freqSource[loc.source]++;
	}
	for (const auto& pair : freqSource) {
		std::cout << pair.first << ": " << pair.second << std::endl;
	}

	std::unordered_map<int, int> freqFrequencyMHZ;
	for (auto& loc : locations) {
		freqFrequencyMHZ[loc.frequencyMhz]++;
	}
	for (const auto& pair : freqFrequencyMHZ) {
		std::cout << pair.first << ": " << pair.second << std::endl;
	}


	std::unordered_map<std::string, int> freqPlatformType;
	for (auto& loc : locations) {
		freqPlatformType[loc.platformType]++;
	}
	for (const auto& pair : freqPlatformType) {
		std::cout << pair.first << ": " << pair.second << std::endl;
	}


	std::unordered_map<std::string, int> freqType;
	for (auto& loc : locations) {
		freqType[loc.type]++;
	}
	for (const auto& pair : freqType) {
		std::cout << pair.first << ": " << pair.second << std::endl;
	}

	std::unordered_map<std::string, int> freqDeviceTag;
	for (auto& loc : locations) {
		freqDeviceTag[loc.deviceTag]++;
	}
	for (const auto& pair : freqDeviceTag) {
		std::cout << pair.first << ": " << pair.second << std::endl;
	}


	std::unordered_map<unsigned long long, int> freqMac;
	for (auto& loc : locations) {
		freqMac[loc.mac]++;
	}
	for (const auto& pair : freqMac) {

		if (pair.second > 2000) {
			for (int i = 0; i < 6; i++) {
				std::cout << std::setfill('0') << std::setw(2) << std::hex << ((pair.first >> (i * 8)) & 0xff);
				if (i < 5) {
					std::cout << ':';
				}
			}

			std::cout << ": " << std::dec << pair.second << std::endl;
		}
	}


	fastestVelocity = 0;
	for (auto& loc : locations) {
		if (loc.velocity < velocityBins) {
			histoVelocity[loc.velocity]++;
		}
		if (loc.velocity > fastestVelocity) {
			fastestVelocity = loc.velocity;
		}
	}

	for (int i = 0; i < velocityBins; i++) {
		if (histoVelocity[i]) {
			//printf("%i %i (%f%%)\n", i, stats.histoVelocity[i], 100.0f * (float)stats.histoVelocity[i] / (float)stats.numberOfLocations);
		}
	}


}

void Statistics::GenerateStatistics(const std::vector<Location>& locations, std::function<bool(const Location&)> filter)
{
	std::unordered_map<std::string, int> sourceFrequency;
	for (const auto& location : locations)
	{
		if (filter(location))
		{
			sourceFrequency[location.source]++;
		}
	}
	for (const auto& [source, frequency] : sourceFrequency)
	{
		std::cout << source << ": " << frequency << std::endl;
	}

}
