#pragma once
#include <time.h>
#include <string>



namespace MyTimeZone {

	constexpr int secondsperday = 60 * 60 * 24;

	namespace FormatFlags {
		const int	DEFAULT = 0;
		const int	SHOW_TIME = (1 << 0);
		const int	SHOW_DAY = (1 << 1);
		const int	TWO_NUMBER_YEAR = (1 << 2);
		const int	TWELVE_HOUR_CLOCK = (1 << 3);
		const int	MDY = (1 << 4);
		const int	DMY = (1 << 5);
		const int	YMD = DEFAULT;
		const int	MONTH_SHORT = (1 << 6);
		const int	MONTH_LONG = (1 << 7);//not used yet
		const int	MONTH_NUM = DEFAULT;
	};

	const std::string daynames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	const std::string daynameslong[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
	const std::string monthnames[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

	class DateFormatOptions {
	private:
		int combinedFlags;
	public:
		int GetDateCustomFormat();	//this includes all formatting types (order, and day length etc), I have custom in there as there's a windows function with the same name
		int GetDateOrder();
		void SetDateOrder(int dOrder);
		void SetMonthFormat(int mf);

	};

	unsigned long AsLocalTime(unsigned long unixtime);
	long tz_offset_second(time_t t);
	unsigned long AdjustBasedOnLongitude(unsigned long unixtime, float longitude);
	std::string FormatUnixTime(unsigned long unixtime, int flags);

	unsigned long GetYearFromTimestamp(unsigned long unixtime);

	int GetDayOfWeek(unsigned long unixtime);
	int GetDaySince2010(unsigned long unixtime);



	std::string DisplayBestTimeUnits(unsigned long seconds);
	
}