#pragma once
#include <time.h>
#include <string>
namespace MyTimeZone {
	unsigned long FixToLocalTime(unsigned long unixtime);
	long tz_offset_second(time_t t);
	unsigned long AdjustBasedOnLongitude(unsigned long unixtime, float longitude);
	std::string FormatUnixTime(unsigned long unixtime);
}