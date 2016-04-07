/*14-5 在文件整体上加锁*/

#include <unistd.h>
#include <fcntl.h>

int lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;//整个文件

	return(fcntl(fd, F_SETLK, &fl));
}