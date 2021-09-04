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

    naivetz = (int)((longitude + 7.5f) / 15.0f);
 
    return unixtime+naivetz*3600;
}

std::string MyTimeZone::FormatUnixTime(unsigned long unixtime, int flags)
{
    std::string output;


    
    struct std::tm *corrected;

    time_t t;
    t = unixtime;
    
    corrected = gmtime(&t);

    std::string year, mon, mday, wday, delim;

    year = std::to_string(corrected->tm_year + 1900);
    
    delim = "-";
    if (flags & FormatFlags::TEXT_MONTH) {
        mon = monthnames[corrected->tm_mon];
        delim = " ";
    }
    else {
        mon = std::to_string(corrected->tm_mon + 1);
    }

    mday = std::to_string(corrected->tm_mday);

    if (flags & FormatFlags::SHOW_DAY) {
        wday = daynames[corrected->tm_wday] + " ";
    }
    else {
        wday = "";
    }


    if (flags & FormatFlags::DMY) { //nz format
        output = wday + mday + delim + mon + delim + year;
    }
    else if (flags & FormatFlags::MDY) { //us format
        output = wday + mon + delim + mday + delim + year;
    }
    else { //y-m-d
        output = wday + year + delim + mon + delim + mday;
    }

    if (flags & FormatFlags::SHOW_TIME) {
        output = output + ((corrected->tm_hour > 9) ? " " : " 0") + std::to_string(corrected->tm_hour) + ((corrected->tm_min > 9) ? ":" : ":0") + std::to_string(corrected->tm_min);
    }
    return output;
}

unsigned long MyTimeZone::GetYearFromTimestamp(unsigned long unixtime)
{
    
    unixtime += 31536000;	//increase the date, so we start on a non-leap, after a leap year
    const unsigned long fouryears = 31536000 * 3 + 31622400;	//365,365,365,366 days
    unsigned long olympiad = unixtime / fouryears;	//which group of four years
    unsigned long remainder = (unixtime - (olympiad * fouryears)) / 31536000;
    if (remainder > 3) remainder = 3;

    unsigned long yearcalc = olympiad * 4 + remainder + 1969;	//from 1969 as we went forward a year previously
    
    return yearcalc;
}