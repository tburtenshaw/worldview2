#include "options.h"

GlobalOptions::GlobalOptions()
{
	seconds = 0.0f;	//an easy way for anyone to get the seconds counter (it's set in main loop by ImGui IO)

	showPaths = false;
	showPoints = false;
	showHeatmap = true;

	linewidth = 2.0;
	cycleSeconds = 3600.0;

	earliestTimeToShow = 0;
	latestTimeToShow = 2147400000;

	pointdiameter = 5.0f;
	pointalpha = 0.5f;

	showHighlights = true;
	secondsbetweenhighlights = 2.0f;
	minutestravelbetweenhighlights = 60.0f;

	colourby = 1;

	minimumaccuracy = 24;
	palette = 1;

	gaussianblur = 0.0f;

	predictpath = false;
	heatmapmaxvalue = 500;

	debug = 0.0f;

}

void GlobalOptions::ShowHeatmap(bool hideOthers)
{
	this->showHeatmap = true;
	if (hideOthers) {
		this->showPaths = false;
		this->showPoints = false;
	}
}

void GlobalOptions::ShowPoints(bool hideOthers)
{
	this->showPoints = true;
	if (hideOthers) {
		this->showPaths = false;
		this->showHeatmap = false;
	}
}