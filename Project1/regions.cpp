#include <vector>
#include <string>

#include "header.h"
#include "nswe.h"
#include "regions.h"
#include "mytimezone.h"

const int secondsperday = 60 * 60 * 24;
int Region::numberOfNextRegion = 0;
/*
Region::Region()
{
	SetNSWE(-10, 10, -10, 10);
	earliestday = 0;
	latestday = 0;
	
}
*/

Region::Region(float n, float s, float w, float e)
{
	id = numberOfNextRegion;
	displayname = "Region " + std::to_string(id);
	shouldShowWindow = true;
	toDelete = false;
	//printf(displayname.c_str());

	numberOfNextRegion++;
	nswe.north = n;
	nswe.south = s;
	nswe.west = w;
	nswe.east = e;

	completed = 0;
	earliestday = 0;
	latestday = 0;
	minimumsecondstobeincludedinday = 0;

	memset(&hours, 0, sizeof(hours));
	memset(&dayofweeks, 0, sizeof(dayofweeks));
}

Region::~Region()
{
	
}

float Region::GetHoursInRegion()
{
	float h;
	h = (float)totalsecondsinregion;
	h /= 3600;
	return h;
}

void Region::SetNSWE(NSWE* sourceNSWE)
{
	nswe = *sourceNSWE;
}

void Region::SetNSWE(float n, float s, float w, float e)
{
	if (n > s) {	//check these are the right way around
		nswe.north = n;
		nswe.south = s;
	}
	else {	//otherwise flip them
		nswe.north = s;
		nswe.south = n;
	}

	if (w < e) {
		nswe.west = w;
		nswe.east = e;
	}
	else {
		nswe.west = e;
		nswe.east = w;
	}
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

	unsigned int e = GetDaySince2010(lh->earliesttimestamp);
	unsigned int l = GetDaySince2010(lh->latesttimestamp);

	earliestday = MAX_DAY_NUMBER;

	for (unsigned int i = e; (i <= l) && (i< MAX_DAY_NUMBER); i++) {
		if (daynumbersince2010[i] > 0) {
			if (i < earliestday) { earliestday = i; }
			latestday = i;	//this will be overwritten mulitple times
			//printf("%i %i %i %s\n", i, i * 60 * 60 * 24 + 1262304000, daynumbersince2010[i], FormatUnixTime(i * 60 * 60 * 24 + 1262304000).c_str());
		}
	}
	//printf("%i %i\n", earliestday, latestday);
	
	return;
}

void Region::FillVectorWithDates(std::vector<std::string>& list)
{
	numberofdays = 0;	//this is used for stats, not for the algorithm below
	
	unsigned int inrun = 0;

	if (latestday >= MAX_DAY_NUMBER-1) {
		latestday = 0;
	}
	for (int i = earliestday; i <= latestday+1; i++) {
		if (daynumbersince2010[i] > minimumsecondstobeincludedinday) {	//at least fifteen minutes
			numberofdays++;
			if (inrun == 0) {
				list.push_back(MyTimeZone::FormatUnixTime(i * secondsperday + 1262304000,MyTimeZone::FormatFlags::DEFAULT));
				inrun = i;
			}
		}
		else {
			if ((inrun < (i - 1)) && (inrun > 0)) {

				if (daynumbersince2010[i - 1] > minimumsecondstobeincludedinday) {
					std::string last;
					last = list.back();

					list.back() = last + " to " + MyTimeZone::FormatUnixTime((i - 1) * secondsperday + 1262304000, MyTimeZone::FormatFlags::DEFAULT);
					//list.push_back(" to " + MyTimeZone::FormatUnixTime((i - 1) * secondsperday + 1262304000));
				}
				inrun = 0;
			}
		}

	}
	if ((inrun < latestday) && (inrun > 0)) {
		list.push_back(" to " + MyTimeZone::FormatUnixTime((latestday)*secondsperday + 1262304000, MyTimeZone::FormatFlags::DEFAULT));
	}
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
	return ((unixtime / secondsperday) + 4) % 7;
}

int Region::GetDaySince2010(unsigned long unixtime)
{
	unsigned long u;
	u = (unixtime - 1262304000) / secondsperday;
	if (u >= MAX_DAY_NUMBER) {
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