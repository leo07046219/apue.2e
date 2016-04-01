/*12-1 以分离状态创建线程*/

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

    /*分离状态*/
	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err == 0)
    {
        err = pthread_create(&tid, &attr, fn, arg);
    }

    /*返回值检测没有意义，最多只能打印一下，
    因为相关的操作就这一个函数(pthread_attr_destroy)，没法补救。*/
	pthread_attr_destroy(&attr);

	return(err);
}