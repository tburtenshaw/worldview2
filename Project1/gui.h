#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
class GlobalOptions;
struct RectDimension;
class MainViewport;

#include <string>
#include <map>
#include "header.h"
#include "guiatlas.h"




namespace Gui {
	void ShowLoadingWindow(LocationHistory* lh);
	
	void MakeGUI(LocationHistory* lh, MainViewport * vp);
	bool ToolbarButton(GuiAtlas atlas, enum class Icon icon);
	
	void DebugWindow(LocationHistory* lh, MainViewport* vp);
	//void OptionsWindow
	void ToolbarWindow(LocationHistory* lh);
	void InfoWindow(LocationHistory* lh, MainViewport* vp);

	void PointsOptions(LocationHistory* lh);
	void HeatmapOptions();
	void DateSelect(LocationHistory* lh);
	
	void ShowRegionInfo(Region* r);
	void DayHistogram(Region* r, float height);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(const double dpp);
	bool ChooseFileToOpen(LocationHistory * lh);
	std::wstring ChooseFileToSave(LocationHistory* lh);
}