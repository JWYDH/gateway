#pragma once
#include <stdint.h>

namespace jw
{

//----------------------
// time functions
//----------------------

int64_t time_tick();

int64_t utc_time_now();

int64_t local_time_now();

void time_tostring(time_t t, char *time_dest, size_t max_size, const char *format);

int64_t time_encode(char *str);

} // namespace jw
