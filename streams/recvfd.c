/*17-13 STREAMS管道的recv_fd函数*/

#include "apue.h"
#include <stropts.h>

/*
 * Receive a file descriptor from another process (a server).
 * In addition, any data received from the server is passed
 * to (*userfunc)(STDERR_FILENO, buf, nbytes).  We have a
 * 2-byte protocol for receiving the fd from send_fd().
 */
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t))
{
	int					newfd = 0, nread = 0, flag = 0, status = 0;
	char				*ptr = NULL;
	char				buf[MAXLINE];
	struct strbuf		dat;
	struct strrecvfd	recvfd;

	status = -1;
    memset((char *)buf, 0, sizeof(buf));
    memset((char *)&dat, 0, sizeof(dat));
    memset((char *)&recvfd, 0, sizeof(recvfd));

	for ( ; ; ) 
    {
		dat.buf = buf;
		dat.maxlen = MAXLINE;
		flag = 0;

        if (getmsg(fd, NULL, &dat, &flag) < 0)
        {
            err_sys("getmsg error");
        }

		nread = dat.len;

		if (0 == nread) 
        {
			err_ret("connection closed by server");
			return(-1);
		}

		/*
		 * See if this is the final data with null & status.
		 * Null must be next to last byte of buffer, status
		 * byte is last byte.  Zero status means there must
		 * be a file descriptor to receive.
         接收双字节协议及fd
		 */
		for (ptr = buf; ptr < &buf[nread]; ) 
        {
			if (0 == *ptr++) 
            {
                if (ptr != &buf[nread - 1])
                {
                    err_dump("message format error");
                }

                status = *ptr & 0xFF;   /* prevent sign extension */

                if (0 == status) 
                {
                    if (ioctl(fd, I_RECVFD, &recvfd) < 0)
                    {
                        return(-1);
                    }
					newfd = recvfd.fd;	/* new descriptor */
				} 
                else 
                {
					newfd = -status;
				}

				nread -= 2;
			}
		}
        /*通过用户指定的接收函数接收后续数据*/
        if (nread > 0)
        {
            if ((*userfunc)(STDERR_FILENO, buf, nread) != nread)
            {
                return(-1);
            }
        }

        if (status >= 0)	/* final data has arrived */
        {
            return(newfd);	/* descriptor, or -status */
        }
	}
}
