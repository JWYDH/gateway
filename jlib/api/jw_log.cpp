#include "jw_log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

#include "jw_process.h"
#include "jw_thread.h"
#include "jw_file.h"
#include "jw_time.h"

namespace jw
{

#define LOG_PATH "./logs/"
#define LOG_FILENAME_MAX 256
#define LOG_FILE_MAX_SIZE 512 * 1024 * 1024

class LogFile
{
private:
	static LogFile *singleton_;

public:
	char process_name_[LOG_FILENAME_MAX];
	mutex_t lock_;
	const char *level_name_[LL_MAX];

	LogFile()
	{
		//pid_t process_id = jw::process_get_id();

		//get process name
		char process_name[LOG_FILENAME_MAX] = {0};
		jw::process_get_module_name(process_name, LOG_FILENAME_MAX);

		//create lock
		lock_ = jw::mutex_create();

		// //set level name
		level_name_[LL_DEBUG] = "[D]";
		level_name_[LL_INFO] = "[I]";
		level_name_[LL_WARN] = "[W]";
		level_name_[LL_ERROR] = "[E]";
	}

	~LogFile()
	{
		jw::mutex_destroy(lock_);
	}

	static LogFile &instance()
	{
		return *singleton_;
	}
};

//thread safe init singleton
LogFile *LogFile::singleton_ = new LogFile;

void log_file(LOG_LEVEL level, const char *message, ...)
{
	assert(level >= LL_DEBUG && level < LL_MAX);
	if (level < LL_DEBUG || level >= LL_MAX)
		return;

	LogFile &thefile = LogFile::instance();

	jw::auto_mutex(thefile.lock_);

	//check dir
	if (!jw::file_access(LOG_PATH))
	{
		if (jw::dir_create(LOG_PATH))
		{
			//create log path failed!
			return;
		}
	}

	char file_name[LOG_FILENAME_MAX] = {0};
	char time_buf[32] = {0};

	time_t now = jw::local_time_now();
	jw::time_tostring(now, time_buf, 32, "%s%Y%m%d");
	snprintf(file_name, LOG_FILENAME_MAX, "%s-%s", thefile.process_name_, time_buf);
	
	

	FILE *fp = fopen(file_name, "a");
	if (fp == 0)
	{
		//create the log file first
		fp = fopen(file_name, "w");
	}
	if (fp == 0)
	{
		//open the log file fail
		return;
	}
	
	static const int32_t STATIC_BUF_LENGTH = 2048;

	char log_string[STATIC_BUF_LENGTH] = {0};
	char *p = log_string;

	va_list ptr;
	va_start(ptr, message);
	int len = vsnprintf(p, STATIC_BUF_LENGTH, message, ptr);
	if (len < 0)
	{
		va_start(ptr, message);
		len = vsnprintf(0, 0, message, ptr);
		if (len > 0)
		{
			p = (char *)malloc((size_t)(len + 1));
			va_start(ptr, message);
			vsnprintf(p, (size_t)len + 1, message, ptr);
			p[len] = 0;
		}
	}
	else if (len >= STATIC_BUF_LENGTH)
	{
		p = (char *)malloc((size_t)(len + 1));
		va_start(ptr, message);
		vsnprintf(p, (size_t)len + 1, message, ptr);
		p[len] = 0;
	}
	va_end(ptr);

	jw::time_tostring(now, time_buf, 32, "%Y_%m_%d-%H:%M:%S");

	fprintf(fp, "%s %s [%s] %s\n",
			time_buf,
			thefile.level_name_[level],
			jw::thread_get_current_name(),
			p);
	fclose(fp);

	//print to stand output last
	fprintf(level >= LL_ERROR ? stderr : stdout, "%s %s [%s] %s\n",
			time_buf,
			thefile.level_name_[level],
			jw::thread_get_current_name(),
			p);

	if (p != log_string)
	{
		free(p);
	}
}

} // namespace jw