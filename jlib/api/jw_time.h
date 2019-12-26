#pragma once
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

namespace jw
{

//----------------------
// time functions
//----------------------

int64_t time_tick();

int64_t utc_time_now();

time_t local_time_now();

void time_tostring(time_t t, char *time_dest, size_t max_size, const char *format);

time_t time_encode(char *str);

} // namespace jw
