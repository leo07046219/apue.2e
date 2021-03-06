/*11-2 获得线程的退出状态：pthread_join*/

#include "apue.h"
#include <pthread.h>

void * thr_fn1(void *arg)
{
	printf("thread 1 returning\n");
	return((void *)1);
}

void * thr_fn2(void *arg)
{
	printf("thread 2 exiting\n");
	pthread_exit((void *)2);
}

int main(void)
{
	int			err = 0;
	pthread_t	tid1, tid2;
	void		*tret = NULL;

	err = pthread_create(&tid1, NULL, thr_fn1, NULL);

    if (err != 0)
    {
        err_quit("can't create thread 1: %s\n", strerror(err));
    }

	err = pthread_create(&tid2, NULL, thr_fn2, NULL);
    if (err != 0)
    {
        err_quit("can't create thread 2: %s\n", strerror(err));
    }

	err = pthread_join(tid1, &tret);
    if (err != 0)
    {
        err_quit("can't join with thread 1: %s\n", strerror(err));
    }
	printf("thread 1 exit code %d\n", (int)tret);

	err = pthread_join(tid2, &tret);
    if (err != 0)
    {
        err_quit("can't join with thread 2: %s\n", strerror(err));
    }
	printf("thread 2 exit code %d\n", (int)tret);

	exit(0);
}
/*
1.pthread_create、pthread_exit函数的void指针可以传一个结构体地址，
但必须保证该结构所使用的内存在调用者完成调用以后仍然有效。
*/