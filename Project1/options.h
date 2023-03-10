#pragma once
class GlobalOptions {
private:

public:
	GlobalOptions();

	void ShowHeatmap(bool hideOthers = true);
	void ShowPoints(bool hideOthers = true);

	bool showPaths;
	bool showPoints;
	bool showHeatmap;
	float seconds;

	//display
	unsigned long earliestTimeToShow;
	unsigned long latestTimeToShow;

	//paths
	float linewidth;
	float cycleSeconds;
	int colourby;

	//palettes
	int indexPaletteHour = 0;
	int indexPaletteWeekday = 0;
	int indexPaletteYear = 0;

	//points
	float pointdiameter;
	float pointalpha;
	bool showHighlights;
	float secondsbetweenhighlights;
	float minutestravelbetweenhighlights;

	//heatmap
	int palette; //viridis = 1, inferno = 2
	int minimumaccuracy;
	bool predictpath;
	float gaussianblur;
	float heatmapmaxvalue;
	float debug;

};

// Declare the global variable as extern, defined in winconsole.cpp
extern GlobalOptions globalOptions;