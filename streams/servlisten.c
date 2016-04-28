/*17-3 使用STREAM管道的serv_listen函数*/

#include "apue.h"
#include <fcntl.h>
#include <stropts.h>

/* pipe permissions: user rw, group rw, others rw */
#define	FIFO_MODE  (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

/*
 * Establish an endpoint to listen for connect requests.
 * Returns fd if all OK, <0 on error
 */
int serv_listen(const char *name)
{
	int		tempfd = 0;
	int		fd[2];

    memset(fd, 0, sizeof(fd));

	/*
	 * Create a file: mount point for fattach().
	 */
	unlink(name);
    if ((tempfd = creat(name, FIFO_MODE)) < 0)
    {
        return(-1);
    }
    if (close(tempfd) < 0)
    {
        return(-2);
    }
    if (pipe(fd) < 0)
    {
        return(-3);
    }

	/*
	 * Push connld & fattach() on fd[1].
	 */
	if (ioctl(fd[1], I_PUSH, "connld") < 0) 
    {
		close(fd[0]);
		close(fd[1]);
		return(-4);
	}

    /*一旦STREAMS管道连接到文件系统的名字空间，文件不再可访问。
    打开名字的任意进程能访问响应管道，而不是访问文件。*/
	if (fattach(fd[1], name) < 0) 
    {
		close(fd[0]);
		close(fd[1]);
		return(-5);
	}
	close(fd[1]);           /* fattach holds this end open */

	return(fd[0]);          /* fd[0] is where client connections arrive */
}
/*
1.CONNLD模块保证管道连接的唯一性(？)；
2.通过一个众所周知的名字，建立一个侦听管道，返回管道的读取端；
*/