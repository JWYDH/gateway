#include "jw_time.h"

#include <sys/time.h>

#include <time.h>
#include <string.h>

namespace jw
{

int64_t time_tick()
{
	timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec;
}

int64_t utc_time_now()
{
	struct timeval t;
	gettimeofday(&t, nullptr);
	return t.tv_sec;
}

int64_t local_time_now()
{
	time_t local_time = time(nullptr);
	struct tm tm_now;
	//time zone;
	localtime_r(&local_time, &tm_now);
	local_time = mktime(&tm_now);
	return local_time
}

void time_tostring(time_t t, char *time_dest, size_t max_size, const char *format)
{
	struct tm r;
	localtime_r((const time_t *)&t, &r);
	strftime(time_dest, max_size, format, &r);
}

int64_t time_encode(char *str)
{
	if (str == nullptr || strlen(str) != 14)
	{
		return 0;
	}
	char *p = str;
	while ((*p))
	{
		if ((*p) < '0' || (*p) > '9')
		{
			return 0;
		}
		p++;
	}
	struct tm t;
	t.tm_year = (str[0] - '0') * 1000 + (str[1] - '0') * 100 + (str[2] - '0') * 10 + (str[3] - '0') - 1900;
	t.tm_mon = (str[4] - '0') * 10 + (str[5] - '0') - 1;
	t.tm_mday = (str[6] - '0') * 10 + (str[7] - '0');
	t.tm_hour = (str[8] - '0') * 10 + (str[9] - '0');
	t.tm_min = (str[10] - '0') * 10 + (str[11] - '0');
	t.tm_sec = (str[12] - '0') * 10 + (str[13] - '0');
	t.tm_isdst = 0;
	time_t r = mktime(&t);
	return r;
}

} // namespace jw
