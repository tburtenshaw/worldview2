#pragma once

//forward declaration
class NSWE;
class LocationHistory;

class Region {
private:
	void AddHoursOfDay(unsigned long startt, unsigned long endt);
	
	int GetDayOfWeek(unsigned long unixtime);
	void AddDaysOfWeek(unsigned long startt, unsigned long endt);
public:
	Region();
	Region(float n, float s, float w, float e);
	
	NSWE nswe;

	bool completed;
	unsigned long hours[24];	//seconds in each hour
	unsigned long dayofweeks[7];	//seconds in each day of week
	unsigned long months[12];

	void SetNSWE(NSWE* sourceNSWE);
	void SetNSWE(float n, float s, float w, float e);

	void Populate(LocationHistory* lh);
};