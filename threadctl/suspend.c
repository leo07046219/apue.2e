/*12-6 同步信号处理*/

#include "apue.h"
#include <pthread.h>

int			quitflag;	/* set nonzero by thread */
sigset_t	mask;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  waitloc = PTHREAD_COND_INITIALIZER;

void * thr_fn(void *arg)
{
	int err, signo;

	for (;;) 
    {
		err = sigwait(&mask, &signo);
        if (err != 0)
        {
            err_exit(err, "sigwait failed");
        }
		switch (signo) 
        {
		case SIGINT:
			printf("\ninterrupt\n");
			break;

		case SIGQUIT:
			pthread_mutex_lock(&lock);
			quitflag = 1;
			pthread_mutex_unlock(&lock);
			pthread_cond_signal(&waitloc);
			return(0);

		default:
			printf("unexpected signal %d\n", signo);
			exit(1);
		}
	}
}

int main(void)
{
	int			err = 0;
	sigset_t	oldmask;
	pthread_t	tid;

    /*主线程开始时阻塞SIGINT、SIGQUIT,子线程继承主线程信号屏蔽字，
    sigwait会解除信号的阻塞状态，
    ∴只有一个线程可以用于信号的接收，主线程不必担心来自上述信号的中断*/
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0)
    {
        err_exit(err, "SIG_BLOCK error");
    }

    /*起线程处理信号*/
	err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err != 0)
    {
        err_exit(err, "can't create thread");
    }

	pthread_mutex_lock(&lock);
    while (0 == quitflag)
    {
        pthread_cond_wait(&waitloc, &lock);
    }
	pthread_mutex_unlock(&lock);

	/* SIGQUIT has been caught and is now blocked; do whatever */
	quitflag = 0;

	/* reset signal mask which unblocks SIGQUIT */
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
    {
        err_sys("SIG_SETMASK error");
    }

	exit(0);
}
/*
linux线程
以独立进程实现，使用clone共享资源。
异步信号发送到特定线程时，系统不能选择当前没有阻塞该信号的线程，
线程可能不会注意到该信号。若信号产生于终端驱动程序，这样的信号通知到进程组，可以正常工作；
若试图用kill把信号发送给进程，linux就不能如预期。*/