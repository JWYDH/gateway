#include "jw_log.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

namespace jw
{

#define LOG_PATH "./logs/"

//-------------------------------------------------------------------------------------
struct LogFile
{
	char file_name[256];
	pthread_mutex_t *lock_;
	const char *level_name[LL_MAX];
	LOG_LEVEL level_;
	bool logpath_created_;

	LogFile()
	{
		//get process name
		//char process_name[256] = {0};
		char process_path_name[256] = {0};
		if (readlink("/proc/self/exe", process_path_name, 256) < 0)
		{
			strncpy(process_path_name, "unknown", 256);
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
		//strncpy(module_name, process_name, max_size);

		//get host name
		//char host_name[256];
		//::gethostname(host_name, 256);

		//get process id
		pid_t process_id = ::getpid();

		//log filename patten
		char name_patten[256] = {0};
		snprintf(name_patten, 256, LOG_PATH "%s.%%Y%%m%%d-%%H%%M%%S.%d.log", process_name, process_id);
		jw::local_time_now(file_name, 256, name_patten);

		//create lock
		lock = jw::mutex_create();

		//default level(all level will be writed)
		level_threshold = DEBUG;

		//log path didn't created
		logpath_created = false;

		//set level name
		level_name[LL_DEBUG] = "[D]";
		level_name[LL_INFO] = "[I]";
		level_name[LL_WARN] = "[W]";
		level_name[LL_ERROR] = "[E]";
	}
};

//-------------------------------------------------------------------------------------
static DiskLogFile &_getDiskLog(void)
{
	static DiskLogFile thefile;
	return thefile;
}

//-------------------------------------------------------------------------------------
const char *get_log_filename(void)
{
	DiskLogFile &thefile = _getDiskLog();
	return thefile.file_name;
}

//-------------------------------------------------------------------------------------
void set_log_level(LOG_LEVEL level)
{
	assert(level >= 0 && level <= MAXIMUM);
	if (level < 0 || level > MAXIMUM)
		return;

	DiskLogFile &thefile = _getDiskLog();
	jw::auto_mutex guard(thefile.lock);

	thefile.level_threshold = level;
}

//-------------------------------------------------------------------------------------
void log_file(LOG_LEVEL level, const char *message, ...)
{
	assert(level >= 0 && level < MAXIMUM);
	if (level < 0 || level >= MAXIMUM)
		return;

	DiskLogFile &thefile = _getDiskLog();
	jw::auto_mutex guard(thefile.lock);

	//check the level
	if (level < thefile.level_threshold)
		return;

		//check dir

	if (!thefile.logpath_created && access(LOG_PATH, F_OK) != 0)
	{
		if (mkdir(LOG_PATH, 0755) != 0)
		{
			//create log path failed!
			return;
		}
		thefile.logpath_created = true;
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