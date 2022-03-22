#pragma once
#include <string>
#include <vector>
#define MAX_DAY_NUMBER 5000

//forward declaration
class NSWE;
class LocationHistory;

class Region {
private:
	void AddHoursOfDay(unsigned long startt, unsigned long endt);
	
	int GetDayOfWeek(unsigned long unixtime);
	int GetDaySince2010(unsigned long unixtime);
	void AddDaysOfWeek(unsigned long startt, unsigned long endt);

	unsigned long totalsecondsinregion;
	void CalculateStats(unsigned long startofstay, unsigned long endofstay);
	static int numberOfNextRegion;
public:
	Region() :Region(-10, 10, -10, 10) {};	//I think this is called delegating
	Region(float n, float s, float w, float e);
	~Region();

	NSWE nswe;

	std::string displayname;
	int id;
	RGBA colour;

	bool needsRedraw;	//if the region drawing routine needs to redraw it

	bool completed;
	bool shouldShowWindow;
	bool toDelete;	//the next function that can, will delete this region.

	unsigned long hours[24];	//seconds in each hour
	unsigned long dayofweeks[7];	//seconds in each day of week
	unsigned long daynumbersince2010[MAX_DAY_NUMBER];	//not neccesarily the best way, I was considering stl map
	unsigned long months[12];
	

	unsigned long earliestday;	//stores the locally adjusted unix time of midnight the first day
	unsigned long latestday;

	float GetHoursInRegion();
	
	void SetNSWE(NSWE* sourceNSWE);
	void SetNSWE(float n, float s, float w, float e);

	void Populate(LocationHistory* lh);

	unsigned int minimumsecondstobeincludedinday;
	void FillVectorWithDates(std::vector<std::string> &list);
	unsigned int numberofdays;
	
};

