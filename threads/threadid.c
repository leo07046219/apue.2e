/*11-1 打印线程ID*/

#include "apue.h"
#include <pthread.h>

pthread_t ntid;

void printids(const char *s)
{
	pid_t		pid;
	pthread_t	tid;

	pid = getpid();
	tid = pthread_self();

	printf("%s pid %u tid %u (0x%x)\n", s, (unsigned int)pid, \
	  (unsigned int)tid, (unsigned int)tid);
}

void * thr_fn(void *arg)
{
	printids("new thread: ");
	return((void *)0);
}

int main(void)
{
	int		err = 0;

	err = pthread_create(&ntid, NULL, thr_fn, NULL);

    if (err != 0)
    {
        err_quit("can't create thread: %s\n", strerror(err));
    }

	printids("main thread:");

	sleep(1);/*等待子线程退出*/

	exit(0);
}
/*
1.linux通过clone系统调用实现伪线程创建，实为子进程；
2.主线程的输出基本出现在新建线程输出之前，但linux却不是这样，不能在线程调度上做出任何假设；
*/