#include "jw_api.h"

namespace jw
{

bool dir_create(const char *dir)
{
    bool ret = false;
    if (dir_create(dir))
    {
        ret = true;
    }
    else
    {
        ret = (MKDIR(dir)) == 0;
    }

    return ret;
}

bool file_access(const char *filename)
{
    return ACCESS(filename, 0) == 0;
}


pid_t process_get_id(void)
{
#ifdef CY_SYS_WINDOWS
	return (pid_t)::GetCurrentProcessId();
#else
	return ::getpid();
#endif
}

void process_get_module_name(char* module_name, size_t max_size)
{
#ifdef CY_SYS_WINDOWS
	char process_path_name[MAX_PATH] = { 0 };
	::GetModuleFileName(::GetModuleHandle(0), process_path_name, MAX_PATH);
	strncpy(module_name, ::PathFindFileNameA(process_path_name), max_size);
#else

	#ifdef CY_SYS_MACOS
		char process_path_name[PROC_PIDPATHINFO_MAXSIZE] = { 0 };
		proc_pidpath(process_get_id(), process_path_name, PROC_PIDPATHINFO_MAXSIZE);
	#else
		char process_path_name[256] = { 0 };
		if (readlink("/proc/self/exe", process_path_name, 256)<0) {
			strncpy(process_path_name, "unknown", 256);
		}
	#endif
	
	const char* process_name = strrchr(process_path_name, '/');
	if (process_name != 0) process_name++;
	else process_name = "unknown";

	strncpy(module_name, process_name, max_size);
#endif

}

} // namespace jw

