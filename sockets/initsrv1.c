/*16-3 服务器初始化套接字端点*/

#include "apue.h"
#include <errno.h>
#include <sys/socket.h>

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
	int fd = 0;
	int err = 0;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
    {
        return(-1);
    }

	if (bind(fd, addr, alen) < 0) 
    {
		err = errno;
		goto errout;
	}

	if (type == SOCK_STREAM || type == SOCK_SEQPACKET) 
    {
		if (listen(fd, qlen) < 0) 
        {
			err = errno;
			goto errout;
		}
	}
	return(fd);

errout:
	close(fd);
	errno = err;
	return(-1);
}
/*
服务器进程用以分配和初始化套接字的函数
*/