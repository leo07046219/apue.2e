/*11-4 线程清理处理程序*/

#include "apue.h"
#include <pthread.h>

void cleanup(void *arg)
{
	printf("cleanup: %s\n", (char *)arg);
}

void * thr_fn1(void *arg)
{
	printf("thread 1 start\n");
	pthread_cleanup_push(cleanup, "thread 1 first handler");
	pthread_cleanup_push(cleanup, "thread 1 second handler");
	printf("thread 1 push complete\n");

    if (arg)
    {
        return((void *)1);
    }

	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

    /*从执行结果可以看出，只调用了线程2的清理处理程序，
    如果线程从它的启动例程中返回而终止，那它的清理处理程序就不会被调用*/
	return((void *)1);
}

void * thr_fn2(void *arg)
{
	printf("thread 2 start\n");
	pthread_cleanup_push(cleanup, "thread 2 first handler");
	pthread_cleanup_push(cleanup, "thread 2 second handler");
	printf("thread 2 push complete\n");

    if (arg)
    {
        pthread_exit((void *)2);
    }

	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

	pthread_exit((void *)2);
}

int main(void)
{
	int			err = 0;
	pthread_t	tid1, tid2;
	void		*tret = NULL;

	err = pthread_create(&tid1, NULL, thr_fn1, (void *)1);
    if (err != 0)
    {
        err_quit("can't create thread 1: %s\n", strerror(err));
    }

	err = pthread_create(&tid2, NULL, thr_fn2, (void *)1);
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
1.线程通过pthread_cleanup_push/pop,执行、注册 线程退出时需要调用的函数，先进后出

../apue.2e/threads$ ./cleanup
thread 2 start
thread 2 push complete
cleanup: thread 2 second handler
cleanup: thread 2 first handler
thread 1 start
thread 1 push complete
thread 1 exit code 1
thread 2 exit code 2

2.默认的终止状态会保存到pthread_join，如果线程已经处于分离状态，线程的底层存储资源可以在线程终止时立即被收回，
但此时不能用pthread_join函数等待它终止；

3.分离状态：
3.1.一个分离的线程是不能被其他线程回收或杀死的，它的存储器资源在它终止时由系统自动释放；
3.2.线程的分离状态决定一个线程以什么样的方式来终止自己。在默认情况下线程是非分离状态的，这种情况下，原有的线程等待创建的线程结束。
*/