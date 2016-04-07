/*14-4 死锁检测实例*/

#include "apue.h"
#include <fcntl.h>

static void lockabyte(const char *name, int fd, off_t offset)
{
    if (writew_lock(fd, offset, SEEK_SET, 1) < 0)
    {
        err_sys("%s: writew_lock error", name);
    }

	printf("%s: got the lock, byte %ld\n", name, offset);
}

int main(void)
{
	int		fd = 0;
	pid_t	pid;

	/*
	 * Create a file and write two bytes to it.
	 */
    if ((fd = creat("templock", FILE_MODE)) < 0)
    {
        err_sys("creat error");
    }

    if (write(fd, "ab", 2) != 2)
    {
        err_sys("write error");
    }

	TELL_WAIT();

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == pid) 
    {			
        /* child */
		lockabyte("child", fd, 0);
		TELL_PARENT(getppid());
		WAIT_PARENT();
		lockabyte("child", fd, 1);
	} 
    else 
    {						
        /* parent */
		lockabyte("parent", fd, 1);
		TELL_CHILD(pid);
		WAIT_CHILD();
		lockabyte("parent", fd, 0);
	}

	exit(0);
}
/*
apue.2e/lock$ ./deadlock
parent: got the lock, byte 1
child: got the lock, byte 0
child: writew_lock error: Resource deadlock avoided
parent: got the lock, byte 0

内核可以检测到死锁。？
*/