#pragma once

namespace jw
{

enum LOG_LEVEL
{
	LL_DEBUG,
	LL_INFO,
	LL_WARN,
	LL_ERROR,
	LL_MAX,
};

//----------------------
// log api
//----------------------

//log to a disk file
//filename = process_name.date-time24h.hostname.pid.log
// like "test.20150302-1736.server1.63581.log"
// the time part is the time(LOCAL) of first log be written
void log_file(LOG_LEVEL level, const char *message, ...);

//get current log filename
const char *get_log_filename(void);

//set the a global log level, default is DEBUG,
//all the log message lower than this level will be ignored
void set_log_level(LOG_LEVEL level);

} // namespace jw

//userful macro
#ifdef JW_ENABLE_LOG
#define JW_LOG jw::log_file
#else
#define JW_LOG (void)
#endif