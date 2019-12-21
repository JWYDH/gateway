#pragma once
#include "jw_api.h"

namespace jw
{
	pid_t get_process_id(void)
	{
#ifdef OS_WINDOWS
		return (pid_t)::GetCurrentProcessId();
#else
		return ::getpid();
#endif
	}

	void get_filename(char* module_name, size_t max_size)
	{
#ifdef OS_WINDOWS
		char process_path_name[260] = { 0 };
		::GetModuleFileName(::GetModuleHandle(0), process_path_name, 260);
		strncpy(module_name, ::PathFindFileNameA(process_path_name), max_size);
#else
		char process_path_name[256] = { 0 };
		if (readlink("/proc/self/exe", process_path_name, 256) < 0) {
			strncpy(process_path_name, "unknown", 256);
		}

		const char* process_name = strrchr(process_path_name, '/');
		if (process_name != 0) {
			process_name++;
		}
		else {
			process_name = "unknown";
		}
		strncpy(module_name, process_name, max_size);
#endif
	}
}