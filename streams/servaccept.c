/*17-4 使用STREAMS管道的serv_accept函数，在侦听管道上等待客户端连接到服务器*/

#include "apue.h"
#include <stropts.h>

/*
 * Wait for a client connection to arrive, and accept it.
 * We also obtain the client's user ID.
 * Returns new fd if all OK, <0 on error.
 */
int serv_accept(int listenfd, uid_t *uidptr)
{
	struct strrecvfd	recvfd;

    memset((char *)&recvfd, 0, sizeof(recvfd));

    /*当客户端连接请求到达时，系统自动创建一个新的STREAM管道，
    返回的recvfd结构包含管道fd和用户id*/
    if (ioctl(listenfd, I_RECVFD, &recvfd) < 0)
    {
        return(-1);		            /* could be EINTR if signal caught */
    }

    if (uidptr != NULL)
    {
        *uidptr = recvfd.uid;       /* effective uid of caller */
    }

	return(recvfd.fd);              /* return the new descriptor */
}
