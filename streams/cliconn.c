/*17-5 用STREAMS管道的cli_conn函数连接服务器，获取文件描述符*/

#include "apue.h"
#include <fcntl.h>
#include <stropts.h>

/*
 * Create a client endpoint and connect to a server.
 * Returns fd if all OK, <0 on error.
 */
int
cli_conn(const char *name)
{
	int		fd;

	/* open the mounted stream */
	if ((fd = open(name, O_RDWR)) < 0)
		return(-1);
	if (isastream(fd) == 0) {
		close(fd);
		return(-2);
	}
	return(fd);
}
