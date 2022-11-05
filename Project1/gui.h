#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
class GlobalOptions;
struct RectDimension;
class MainViewport;

#include <string>


namespace Gui {
	void ShowLoadingWindow(LocationHistory* lh);
	
	void MakeGUI(LocationHistory* lh, GlobalOptions* options, MainViewport * vp);
	void ShowRegionInfo(Region* r, GlobalOptions* options);
	void DayHistogram(Region* r, GlobalOptions* options, float height);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(MainViewport* vp);
	bool ChooseFileToOpen(LocationHistory * lh);
	std::wstring ChooseFileToSave(LocationHistory* lh);
}