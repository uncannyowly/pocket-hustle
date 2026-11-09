#include "ui/switcher.h"

#include <fcntl.h>
#include <unistd.h>

int switcher_request(const char *path)
{
	int fd;

	if (!path || !path[0])
		return 0;
	fd = open(path, O_WRONLY | O_CREAT, 0644);
	if (fd < 0)
		return 0;
	close(fd);
	return 1;
}
