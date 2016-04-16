/*14-11 readn和writen函数*/

#include "apue.h"

ssize_t                     /* Read "n" bytes from a descriptor  */

readn(int fd, void *ptr, size_t n)
{
	size_t		nleft;
	ssize_t		nread;

    assert(ptr != NULL);

	nleft = n;

	while (nleft > 0) 
    {
		if ((nread = read(fd, ptr, nleft)) < 0) 
        {
            if (nleft == n)
            {
                return(-1); /* error, return -1 一个字节都没读到，返回出错*/
            }
            else
            {
                break;      /* error, return amount read so far 都到一些字节，返回已读数目*/
            }
		} 
        else if (0 == nread) 
        {
			break;          /* EOF 读完*/
		}

		nleft -= nread;
		ptr   += nread;
	}

	return(n - nleft);      /* return >= 0 */
}
/*
1.对于管道、FIFO以及某些终端、网络和STREAM设备，一次read/write的字节数可能小于指定值，谢需要多次调用read/write直至达到指定值；
*/