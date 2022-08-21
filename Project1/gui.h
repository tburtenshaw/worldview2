#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
class GlobalOptions;
struct RectDimension;


#include <string>


namespace Gui {
	void ShowLoadingWindow(LocationHistory* lh);
	void MakeGUI(LocationHistory* lh);
	void ShowRegionInfo(Region* r, GlobalOptions* options);
	void DayHistogram(Region* r, GlobalOptions* options, float height);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(NSWE* nswe, RectDimension rect);
	bool ChooseFileToOpen(LocationHistory * lh);
	std::wstring ChooseFileToSave(LocationHistory* lh);
}