#include "jw_log.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

namespace jw
{

#define LOG_PATH "./logs/"
#define LOG_MAX_PATH 256

class LogFile
{
public:
	LOG_LEVEL level_;
	pthread_mutex_t *lock_;
	const char *level_name[LL_MAX];
	char file_name[LOG_MAX_PATH];
	bool logpath_created_;

	LogFile()
	{
		//default level(all level will be writed)
		level_ = LL_DEBUG;

		//create lock
		lock_ = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		::pthread_mutex_init(lock_, 0);

		//set level name
		level_name[LL_DEBUG] = "[D]";
		level_name[LL_INFO] = "[I]";
		level_name[LL_WARN] = "[W]";
		level_name[LL_ERROR] = "[E]";

		//pid_t process_id = ::getpid();

		char process_path_name[LOG_MAX_PATH] = {0};
		if (readlink("/proc/self/exe", process_path_name, LOG_MAX_PATH) < 0)
		{
			strncpy(process_path_name, "unknown", LOG_MAX_PATH);
		}
		const char *process_name = strrchr(process_path_name, '/');
		if (process_name != 0)
		{
			process_name++;
		}
		else
		{
			process_name = "unknown";
		}

		//log filename 
		snprintf(file_name, LOG_MAX_PATH, LOG_PATH "%s.log", process_name);
		//log path didn't created
		logpath_created_ = false;
	}

	~LogFile()
	{
		::pthread_mutex_destroy(lock_);
		free(lock_);
	}

	static LogFile &instance()
	{
		static LogFile file;
		return file;
	}
};

void log_file(LOG_LEVEL level, const char *message, ...)
{
	assert(level >= LL_DEBUG && level < LL_MAX);
	if (level < LL_DEBUG || level >= LL_MAX)
		return;

	LogFile &thefile = LogFile::instance();

	::pthread_mutex_lock(thefile.lock_);

	//check dir

	if (!thefile.logpath_created_ && access(LOG_PATH, F_OK) != 0)
	{
		if (mkdir(LOG_PATH, 0755) != 0)
		{
			//create log path failed!
			return;
		}
		thefile.logpath_created_ = true;
	}

	FILE *fp = fopen(thefile.file_name, "a");
	if (fp == 0)
	{
		//create the log file first
		fp = fopen(thefile.file_name, "w");
	}
	if (fp == 0)
		return;

	char timebuf[32] = {0};
	jw::local_time_now(timebuf, 32, "%Y_%m_%d-%H:%M:%S");

	static const int32_t STATIC_BUF_LENGTH = 2048;

	char szTemp[STATIC_BUF_LENGTH] = {0};
	char *p = szTemp;
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

	fprintf(fp, "%s %s [%s] %s\n",
			timebuf,
			thefile.level_name[level],
			jw::thread_get_current_name(),
			p);
	fclose(fp);

	//print to stand output last
	fprintf(level >= ERROR ? stderr : stdout, "%s %s [%s] %s\n",
			timebuf,
			thefile.level_name[level],
			jw::thread_get_current_name(),
			p);

	if (p != szTemp)
	{
		free(p);
	}
}

} // namespace jw