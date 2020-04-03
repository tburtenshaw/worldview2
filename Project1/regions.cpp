#include <vector>

#include "header.h"
#include "nswe.h"
#include "regions.h"
#include "mytimezone.h"



Region::Region()
{
	SetNSWE(-10, 10, -10, 10);
	
}

Region::Region(float n, float s, float w, float e)
{
	nswe.north = n;
	nswe.south = s;
	nswe.west = w;
	nswe.east = e;

	completed = 0;
	printf("%i",sizeof(hours));
	memset(&hours, 0, sizeof(hours));
}

float Region::GetHoursInRegion()
{
	float h;
	h = totalsecondsinregion;
	h /= 3600;
	return h;
}

void Region::SetNSWE(NSWE* sourceNSWE)
{
	nswe = *sourceNSWE;
}

void Region::SetNSWE(float n, float s, float w, float e)
{
	nswe.north = n;
	nswe.south = s;
	nswe.west = w;
	nswe.east = e;
}

void Region::Populate(LocationHistory* lh)
{
	using namespace MyTimeZone;
	
	unsigned long startofstay;
	unsigned long endofstay;

	memset(&hours, 0, sizeof(hours));	//set arrays to zero
	memset(&dayofweeks, 0, sizeof(dayofweeks));	//set arrays to zero
	memset(&daynumbersince2010, 0, sizeof(daynumbersince2010));
	totalsecondsinregion = 0;

	bool instay = false;

	for (std::vector<LOCATION>::iterator iter = lh->locations.begin(); iter != lh->locations.end(); ++iter) {

		if (nswe.containspoint(iter->latitude, iter->longitude)) {
			if (instay == false) {
				startofstay = iter->timestamp;
				instay = true;
			}
		}
		else
		{
			if (instay == true) {	//we've been in the zone, and have now come out
				endofstay = iter->timestamp;
				instay = false;
				
				//printf("%i-%i=%i\n", endofstay, startofstay, endofstay - startofstay);
				if (endofstay > startofstay) {
					CalculateStats(FixToLocalTime(startofstay), FixToLocalTime(endofstay));

				}
			}
		}
	}
	if (instay == true) {//if we're still in, we'll end at the last point
		endofstay = lh->locations.back().timestamp;
		CalculateStats(FixToLocalTime(startofstay), FixToLocalTime(endofstay));
		//printf("out %i %i %i\n",startofstay,endofstay);
	}

//	for (int i = 0; i < 24; i++) {
//		printf("%i, %i\n", i, hours[i]);
//	}

//for (int i = 0; i < 7; i++) {
//	printf("d: %i, sec: %i\n", i, dayofweeks[i]);
//}

	for (int i = 0; i < 5000; i++) {
		if (daynumbersince2010[i]>0)	printf("%i %i %i\n", i, i* 60 * 60 * 24 + 1262304000,daynumbersince2010[i]);
	}

	//printf("%f\n", iter->latitude);
	return;
}

void Region::CalculateStats(unsigned long startofstay, unsigned long endofstay)
{
	AddHoursOfDay(startofstay, endofstay);
	AddDaysOfWeek(startofstay, endofstay);
	totalsecondsinregion += endofstay - startofstay;
}

void Region::AddHoursOfDay(unsigned long startt, unsigned long endt) //this needs to be fixed, it overcounts some things
{
	unsigned long firstpartofhour;
	unsigned long currenthour;

	unsigned long t;	//for the loop

	firstpartofhour = 3600 - (startt % 3600);

	currenthour = (startt / 3600) % 24;
	//printf("%i. %i:%i\n", startt, firstpartofhour, currenthour);

	hours[currenthour] += firstpartofhour;
	

	for (t = startt + firstpartofhour; t < endt; t += 3600) {
		currenthour = (t / 3600) % 24;
		//printf("%i. %i:3600\n", t, currenthour);
		hours[currenthour] += 3600;

	}

	currenthour = (t / 3600) % 24;
	//printf("end %i. %i:%i\n", endt, endt - t + 3600, currenthour);
	hours[currenthour] += endt - t + 3600;
	
	return;
}

int Region::GetDayOfWeek(unsigned long unixtime)
{
	const int secondsperday = 60 * 60 * 24;

	return ((unixtime / secondsperday) + 4) % 7;
}

int Region::GetDaySince2010(unsigned long unixtime)
{
	const int secondsperday = 60 * 60 * 24;

	unsigned long u;
	u = (unixtime - 1262304000) / secondsperday;
	if (u >= 10000) {
		u = 0;
	}

	return u;
}

void Region::AddDaysOfWeek(unsigned long startt, unsigned long endt)
{
	unsigned long firstpartofweek;
	unsigned long currentdayofweek;
	unsigned long enddayofweek;

	unsigned long t;	//for the loop
	const int secondsperday = 60 * 60 * 24;

	if (endt <= startt)	return;	//don't count negatives or zeros

	currentdayofweek = GetDayOfWeek(startt);

	//first, if both the end and start time are the same day
	if (endt - startt <= secondsperday) {
		enddayofweek = GetDayOfWeek(endt);
		if (enddayofweek == currentdayofweek) {
			dayofweeks[currentdayofweek] += endt - startt;	//all the seconds go there
			daynumbersince2010[GetDaySince2010(endt)] += endt - startt;
			return;
		}
	}

	unsigned long firstwholeday;
	unsigned long notintheweek;
	unsigned long lastwholeday;

	firstwholeday = (startt / secondsperday) * secondsperday;
	notintheweek = (startt % secondsperday);
	if (notintheweek > 0) { firstwholeday += secondsperday; }

	firstpartofweek = secondsperday - (notintheweek);

	if (firstpartofweek == secondsperday) {
		firstpartofweek = 0;
	}

	dayofweeks[currentdayofweek] += firstpartofweek;
	daynumbersince2010[GetDaySince2010(startt)] += firstpartofweek;

	//now we'll get the last part of the week
	lastwholeday = (endt / secondsperday) * secondsperday;
	currentdayofweek = GetDayOfWeek(lastwholeday) ;
	dayofweeks[currentdayofweek] += endt - lastwholeday;
	daynumbersince2010[GetDaySince2010(lastwholeday)] += endt - lastwholeday;


	//now the middle part
	for (t = firstwholeday; t < lastwholeday; t += secondsperday) {
		currentdayofweek = GetDayOfWeek(t);
		dayofweeks[currentdayofweek] += secondsperday;
		daynumbersince2010[GetDaySince2010(t)] += secondsperday;
	}
	return;
}