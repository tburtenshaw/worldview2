#include "mytimezone.h"
#include <time.h>
#include <string>
#pragma warning(disable : 4996)

unsigned long MyTimeZone::FixToLocalTime(unsigned long unixtime)
{
	return unixtime+tz_offset_second(unixtime);
}

//thanks https://stackoverflow.com/questions/32424125/c-code-to-get-local-time-offset-in-minutes-relative-to-utc
long MyTimeZone::tz_offset_second(time_t t) {   
    struct tm local = *localtime(&t);
    struct tm utc = *gmtime(&t);
    long diff = ((local.tm_hour - utc.tm_hour) * 60 + (local.tm_min - utc.tm_min))
        * 60L + (local.tm_sec - utc.tm_sec);
    int delta_day = local.tm_mday - utc.tm_mday;
    // If |delta_day| > 1, then end-of-month wrap 
    if ((delta_day == 1) || (delta_day < -1)) {
        diff += 24L * 60 * 60;
    }
    else if ((delta_day == -1) || (delta_day > 1)) {
        diff -= 24L * 60 * 60;
    }
    return diff;
}

unsigned long MyTimeZone::AdjustBasedOnLongitude(unsigned long unixtime, float longitude)
{
    int naivetz;

    naivetz = ((longitude + 7.5) / 15);
 
    return unixtime+naivetz*3600;
}

std::string MyTimeZone::FormatUnixTime(unsigned long unixtime)
{
    std::string output;

    std::string daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    
    struct std::tm *corrected;

    time_t t;
    t = unixtime;
    
    corrected = gmtime(&t);

    
    output = daynames[corrected->tm_wday]+" "+std::to_string(corrected->tm_year+1900)+"-"+std::to_string(corrected->tm_mon+1) + "-" + std::to_string(corrected->tm_mday);

    return output;
}
