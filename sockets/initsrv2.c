/*16-9 ���õ�ַ���ó�ʼ���׽��ֶ˵�*/

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
����������ֹ��������������ʱ��16-3���������������������ǳ�ʱ��ͨ��TCP��ʵ�ֲ������ͬһ����ַ��
���˵����׽���ѡ��SO_REUSEADDR����Խ��������ơ�
*/