#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
struct RECTDIMENSION;


namespace Gui {
	void ShowLoadingWindow(LocationHistory* lh);
	void MakeGUI(LocationHistory* lh);
	void ShowRegionInfo(Region* r);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(NSWE* nswe, RECTDIMENSION *rect);
	bool ChooseFile(LocationHistory * lh);
}