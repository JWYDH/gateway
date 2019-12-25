#include "jw_process.h"

#include <string.h>
#include <sys/types.h>
#include <unistd.h>


namespace jw
{

int process_get_id(void)
{
	return (int)::getpid();
}

void process_get_module_name(char *module_name, size_t max_size)
{
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
	strncpy(module_name, process_name, max_size);
}

} // namespace jw
