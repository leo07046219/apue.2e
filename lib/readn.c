/*14-11 readn��writen����*/

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
                return(-1); /* error, return -1 һ���ֽڶ�û���������س���*/
            }
            else
            {
                break;      /* error, return amount read so far ����һЩ�ֽڣ������Ѷ���Ŀ*/
            }
		} 
        else if (0 == nread) 
        {
			break;          /* EOF ����*/
		}

		nleft -= nread;
		ptr   += nread;
	}

	return(n - nleft);      /* return >= 0 */
}
/*
1.���ڹܵ���FIFO�Լ�ĳЩ�նˡ������STREAM�豸��һ��read/write���ֽ�������С��ָ��ֵ��л��Ҫ��ε���read/writeֱ���ﵽָ��ֵ��
*/