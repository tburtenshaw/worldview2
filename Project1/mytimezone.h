#pragma once
#include <time.h>
#include <string>



namespace MyTimeZone {

	namespace FormatFlags {
		const int	DEFAULT = 0;
		const int	SHOW_TIME = (1 << 0);
		const int	SHOW_DAY = (1 << 1);
		const int	TWO_NUMBER_YEAR = (1 << 2);
		const int	TWELVE_HOUR_CLOCK = (1 << 3);
		const int	MDY = (1 << 4);
		const int	DMY = (1 << 5);
		const int	TEXT_MONTH = (1 << 6);
	};


	unsigned long FixToLocalTime(unsigned long unixtime);
	long tz_offset_second(time_t t);
	unsigned long AdjustBasedOnLongitude(unsigned long unixtime, float longitude);
	std::string FormatUnixTime(unsigned long unixtime, int flags);

	
}