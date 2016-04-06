/*14-3 测试一个锁状态的函数*/

#include "apue.h"
#include <fcntl.h>

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
	struct flock	lock;

	lock.l_type = type;		/* F_RDLCK or F_WRLCK */
	lock.l_start = offset;	/* byte offset, relative to l_whence */
	lock.l_whence = whence;	/* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len;		/* #bytes (0 means to EOF) */

    if (fcntl(fd, F_GETLK, &lock) < 0)
    {
        err_sys("fcntl error");
    }

    if (F_UNLCK == lock.l_type)
    {
        return(0);		/* false, region isn't locked by another proc */
    }

	return(lock.l_pid);	/* true, return pid of lock owner */
}
/*进程不能使用locktest测试它自己是否在文件的某一部分持有一把锁，
∵调用进程总是用新锁替换旧锁，绝不会阻塞在自己持有的锁上*/