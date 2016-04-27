/*16-9 采用地址复用初始化套接字端点*/

#include "apue.h"
#include <errno.h>
#include <sys/socket.h>

int initserver(int type, const struct sockaddr *addr, socklen_t alen, \
  int qlen)
{
	int fd = 0, err = 0;
	int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
    {
        return(-1);
    }

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
	  sizeof(int)) < 0) 
    {
		err = errno;
		goto errout;
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
当服务器终止并尝试立即重启时，16-3函数不会正常工作，除非超时，通常TCP的实现不允许绑定同一个地址。
幸运的是套接字选项SO_REUSEADDR允许越过这个限制。
*/