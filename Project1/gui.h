#pragma once
//forward declarations
class LocationHistory;
class NSWE;
struct RECTDIMENSION;


namespace Gui {
	void MakeGUI(LocationHistory* lh);
	const char* BestSigFigsFormat(NSWE* nswe, RECTDIMENSION *rect);
}