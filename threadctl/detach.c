/*12-1 �Է���״̬�����߳�*/

#include "apue.h"
#include <pthread.h>

int makethread(void *(*fn)(void *), void *arg)
{
	int				err = 0;
	pthread_t		tid;
	pthread_attr_t	attr;

	err = pthread_attr_init(&attr);
    if (err != 0)
    {
        return(err);
    }

    /*����״̬*/
	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err == 0)
    {
        err = pthread_create(&tid, &attr, fn, arg);
    }

    /*����ֵ���û�����壬���ֻ�ܴ�ӡһ�£�
    ��Ϊ��صĲ�������һ������(pthread_attr_destroy)��û�����ȡ�*/
	pthread_attr_destroy(&attr);

	return(err);
}