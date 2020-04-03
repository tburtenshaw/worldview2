#pragma once
//forward declarations
class LocationHistory;
class NSWE;
class Region;
struct RECTDIMENSION;


namespace Gui {
	void MakeGUI(LocationHistory* lh);
	void ListDatesInRegion(Region* r);
	const char* BestSigFigsFormat(NSWE* nswe, RECTDIMENSION *rect);
}