#pragma once

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

public:
	Region();
	Region(float n, float s, float w, float e);
	
	NSWE nswe;

	bool completed;

	unsigned long hours[24];	//seconds in each hour
	unsigned long dayofweeks[7];	//seconds in each day of week
	unsigned long daynumbersince2010[10000];	//not neccesarily the best way, I was considering stl map
	unsigned long months[12];

	unsigned long earliestday;	//stores the locally adjusted unix time of midnight the first day
	unsigned long lastestday;

	float GetHoursInRegion();
	
	void SetNSWE(NSWE* sourceNSWE);
	void SetNSWE(float n, float s, float w, float e);

	void Populate(LocationHistory* lh);
};