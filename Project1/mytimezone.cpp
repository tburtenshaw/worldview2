#include "mytimezone.h"
#include <ctime>
#include <string>
#include <chrono>


unsigned long MyTimeZone::AsLocalTime(unsigned long unixtime)
{
	return unixtime+tz_offset_second(unixtime);
}

//thanks https://stackoverflow.com/questions/32424125/c-code-to-get-local-time-offset-in-minutes-relative-to-utc
long MyTimeZone::tz_offset_second(time_t t) {   
    
    struct std::tm local;
    struct std::tm utc;

    localtime_s(&local, &t);
    gmtime_s(&utc, &t);
    
    
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


    
    struct std::tm corrected;

    time_t t;
    t = unixtime;
    
    gmtime_s(&corrected, &t);

    std::string year, mon, mday, wday, delim;

    year = std::to_string(corrected.tm_year + 1900);
    
    delim = "-";
    if (flags & FormatFlags::MONTH_SHORT) {
        mon = monthnames[corrected.tm_mon];
        delim = " ";
    }
    else {
        mon = std::to_string(corrected.tm_mon + 1);
    }

    mday = std::to_string(corrected.tm_mday);

    if (flags & FormatFlags::SHOW_DAY) {
        wday = daynames[corrected.tm_wday] + " ";
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
        output = output + ((corrected.tm_hour > 9) ? " " : " 0") + std::to_string(corrected.tm_hour) + ((corrected.tm_min > 9) ? ":" : ":0") + std::to_string(corrected.tm_min);
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

int MyTimeZone::GetDayOfWeek(unsigned long unixtime)
{
    return ((unixtime / secondsperday) + 4) % 7;
}

int MyTimeZone::GetDaySince2010(unsigned long unixtime)
{
     unsigned long u = (unixtime - 1262304000) / secondsperday;

    return u;
}

time_t MyTimeZone::DateWithThisTime(time_t date, int h, int m, int s)
{
    struct std::tm correctedTime;

    localtime_s(&correctedTime, &date);
    
    correctedTime.tm_hour = 0;
    correctedTime.tm_min = 0;
    correctedTime.tm_sec = 0;

    return mktime(&correctedTime);
}

time_t MyTimeZone::AdvanceByDays(time_t date, int daysToAdvance)
{
 
    auto tp = std::chrono::system_clock::from_time_t(date);

    auto days = std::chrono::days(daysToAdvance);
    tp += days;

    // Convert back to unsigned long
    return std::chrono::system_clock::to_time_t(tp);
}


std::string MyTimeZone::DisplayBestTimeUnits(unsigned long seconds)
{
  
    if (seconds > 60 * 60 * 24*365) {   //over one year
        return std::to_string(seconds / (60 * 60 * 24*7)) + " weeks";
    }
    else if (seconds > 60 * 60 * 48) {   //over two days
        return std::to_string(seconds / (60 * 60 * 24)) + " days";
    }

    else if (seconds > 60 * 60) {   //over one hour
        return std::to_string(seconds / (60 * 60)) + " hours";
    }

    else if (seconds > 2*60) {   //over two minutes
        return std::to_string(seconds/60) + " minutes";
    }

    return std::to_string(seconds) + " seconds";
}

int MyTimeZone::DateFormatOptions::GetDateCustomFormat()
{
    return combinedFlags;
}

int MyTimeZone::DateFormatOptions::GetDateOrder()
{
    return combinedFlags;
}

void MyTimeZone::DateFormatOptions::SetDateOrder(int dOrder)
{
    combinedFlags = (combinedFlags &~FormatFlags::DMY & ~FormatFlags::MDY & ~FormatFlags::YMD) |dOrder;
}

void MyTimeZone::DateFormatOptions::SetMonthFormat(int mf)
{
    combinedFlags = (combinedFlags & ~FormatFlags::MONTH_SHORT & ~FormatFlags::MONTH_LONG & ~FormatFlags::MONTH_NUM) | mf;
}
