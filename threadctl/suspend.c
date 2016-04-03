/*12-6 ͬ���źŴ���*/

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

    /*���߳̿�ʼʱ����SIGINT��SIGQUIT,���̼̳߳����߳��ź������֣�
    sigwait�����źŵ�����״̬��
    ��ֻ��һ���߳̿��������źŵĽ��գ����̲߳��ص������������źŵ��ж�*/
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0)
    {
        err_exit(err, "SIG_BLOCK error");
    }

    /*���̴߳����ź�*/
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
linux�߳�
�Զ�������ʵ�֣�ʹ��clone������Դ��
�첽�źŷ��͵��ض��߳�ʱ��ϵͳ����ѡ��ǰû���������źŵ��̣߳�
�߳̿��ܲ���ע�⵽���źš����źŲ������ն����������������ź�֪ͨ�������飬��������������
����ͼ��kill���źŷ��͸����̣�linux�Ͳ�����Ԥ�ڡ�*/