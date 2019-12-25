#include "jw_file.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

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
		ret = (mkdir(dir, 0755) == 0);
	}
	return ret;
}

bool file_access(const char *filename)
{
	return access(filename, 0) == 0;
}

} // namespace jw
