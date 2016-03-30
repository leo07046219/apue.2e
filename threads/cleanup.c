/*11-4 �߳����������*/

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

    /*��ִ�н�����Կ�����ֻ�������߳�2�����������
    ����̴߳��������������з��ض���ֹ�����������������Ͳ��ᱻ����*/
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
1.�߳�ͨ��pthread_cleanup_push/pop,ִ�С�ע�� �߳��˳�ʱ��Ҫ���õĺ������Ƚ����

../apue.2e/threads$ ./cleanup
thread 2 start
thread 2 push complete
cleanup: thread 2 second handler
cleanup: thread 2 first handler
thread 1 start
thread 1 push complete
thread 1 exit code 1
thread 2 exit code 2

2.Ĭ�ϵ���ֹ״̬�ᱣ�浽pthread_join������߳��Ѿ����ڷ���״̬���̵߳ĵײ�洢��Դ�������߳���ֹʱ�������ջأ�
����ʱ������pthread_join�����ȴ�����ֹ��

3.����״̬��
3.1.һ��������߳��ǲ��ܱ������̻߳��ջ�ɱ���ģ����Ĵ洢����Դ������ֹʱ��ϵͳ�Զ��ͷţ�
3.2.�̵߳ķ���״̬����һ���߳���ʲô���ķ�ʽ����ֹ�Լ�����Ĭ��������߳��ǷǷ���״̬�ģ���������£�ԭ�е��̵߳ȴ��������߳̽�����
*/