/*11-9 ʹ����������*/

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

        /*æ�ȹ��������е���Ч����*/
        while (NULL == workq)
        {
            pthread_cond_wait(&qready, &qlock);
        }

		mp = workq;
		workq = mp->m_next;

		pthread_mutex_unlock(&qlock);

		/* now process the message mp �ȵ���Ч����������...*/

	}
}

/*����Ϣ�ŵ���������*/
void enqueue_msg(struct msg *mp)
{
    assert(mp != NULL);

	pthread_mutex_lock(&qlock);
	mp->m_next = workq;
	workq = mp;
	pthread_mutex_unlock(&qlock);

    /*��ȴ��̷߳����ź�ʱ����Ҫռ�л�������ֻҪ�߳̿����ڵ���cond_signal֮ǰ����Ϣ�Ӷ������ϳ���
    �Ϳ������ͷŻ������Ժ�������ⲿ�ֹ�����
    ����whileѭ���н������������Բ���������⣺�߳����������ֶ�����Ϊ�գ�Ȼ�󷵻ؼ����ȴ���
    ������벻���������־���������Ҫ��signalʱռ�л�����*/
	pthread_cond_signal(&qready);
}