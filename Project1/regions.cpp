#include <vector>

#include "header.h"
#include "nswe.h"
#include "regions.h"



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
	unsigned long startofstay;
	unsigned long endofstay;

	memset(&hours, 0, sizeof(hours));	//set arrays to zero
	
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

				AddHoursOfDay(startofstay, endofstay);



				if (endofstay < startofstay) {
					//printf("end:%i %i %f,%f\n", endofstay, iter->timestamp, iter->latitude, iter->longitude);
				}
			}
		}
	}

//	for (int i = 0; i < 24; i++) {
//		printf("%i, %i\n", i, hours[i]);
//	}

	//printf("%f\n", iter->latitude);
	return;
}

void Region::AddHoursOfDay(unsigned long startt, unsigned long endt)
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