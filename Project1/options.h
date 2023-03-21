#pragma once
class GlobalOptions {
private:
	//date formatting
	int dateOrder;
public:
	GlobalOptions();

	//setters
	void ShowHeatmap(bool hideOthers = true);	//if false, it won't affect the other displays
	void ShowPoints(bool hideOthers = true);

	bool IsHeatmapVisible() const;	//is showHeatmap true
	bool IsPointsVisible() const;	//is showPoints true

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
	float gaussianblur;
	float heatmapmaxvalue;
	float heatmapMaxTarget;
	float debug;

	int GetDateCustomFormat();	//this includes all formatting types (order, and day length etc), I have custom in there as there's a windows function with the same name
	int GetDateOrder();
	void SetDateOrder(int dOrder);
	
	void SetHeatmapMaxValue(float maxVal, float delaySeconds = 0.0f);
	float GetHeatmapMaxValue() const;

};
