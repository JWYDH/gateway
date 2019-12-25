#pragma once
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

namespace jw
{
//----------------------
// process functions
//----------------------

//// get current process id
pid_t process_get_id(void);

//// get current process name
void process_get_module_name(char *module_name, size_t max_size);

} // namespace jw
