#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
struct RectDimension;

#include <string>


namespace Gui {
	void ShowLoadingWindow(LocationHistory* lh);
	void MakeGUI(LocationHistory* lh);
	void ShowRegionInfo(Region* r);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(NSWE* nswe, RectDimension rect);
	bool ChooseFileToOpen(LocationHistory * lh);
	std::wstring ChooseFileToSave(LocationHistory* lh);
}