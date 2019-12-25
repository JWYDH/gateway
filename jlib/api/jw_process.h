#pragma once


namespace jw
{
//----------------------
// process functions
//----------------------

//// get current process id
int process_get_id(void);

//// get current process name
void process_get_module_name(char *module_name, size_t max_size);

} // namespace jw
