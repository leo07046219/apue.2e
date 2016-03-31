/*11-9 使用条件变量*/

#include <pthread.h>

struct msg 
{
	struct msg *m_next;
	/* ... more stuff here ... */
};

struct msg *workq = NULL;

pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

void process_msg(void)
{
	struct msg *mp = NULL;

	for (;;) 
    {
		pthread_mutex_lock(&qlock);

        /*忙等工作队列中的有效任务*/
        while (NULL == workq)
        {
            pthread_cond_wait(&qready, &qlock);
        }

		mp = workq;
		workq = mp->m_next;

		pthread_mutex_unlock(&qlock);

		/* now process the message mp 等到有效工作，处理...*/

	}
}

/*把消息放到工作队列*/
void enqueue_msg(struct msg *mp)
{
    assert(mp != NULL);

	pthread_mutex_lock(&qlock);
	mp->m_next = workq;
	workq = mp;
	pthread_mutex_unlock(&qlock);

    /*向等待线程发送信号时不需要占有互斥量。只要线程可以在调用cond_signal之前把消息从队列中拖出，
    就可以在释放互斥量以后再完成这部分工作。
    ∵在while循环中建仓条件，所以不会存在问题：线程醒来，发现队列仍为空，然后返回继续等待；
    如果代码不能容忍这种竞争，就需要在signal时占有互斥量*/
	pthread_cond_signal(&qready);
}