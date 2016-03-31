/*11-8 使用读写锁*/

#include <stdlib.h>
#include <pthread.h>

struct job 
{
	struct job *j_next;
	struct job *j_prev;
	pthread_t   j_id;   /* tells which thread handles this job */
	/* ... more stuff here ... */
};

struct queue 
{
	struct job      *q_head;
	struct job      *q_tail;
	pthread_rwlock_t q_lock;
};

/*
 * Initialize a queue.
 */
int queue_init(struct queue *qp)
{
	int err = 0;

	qp->q_head = NULL;
	qp->q_tail = NULL;

	err = pthread_rwlock_init(&qp->q_lock, NULL);
    if (err != 0)
    {
        return(err);
    }

	/* ... continue initialization ... */

	return(0);
}

/*
 * Insert a job at the head of the queue.
 */
void job_insert(struct queue *qp, struct job *jp)
{
    assert(qp != NULL);
    assert(jp != NULL);

	pthread_rwlock_wrlock(&qp->q_lock);

	jp->j_next = qp->q_head;
	jp->j_prev = NULL;
    if (qp->q_head != NULL)
    {
        qp->q_head->j_prev = jp;
    }
    else
    {
        qp->q_tail = jp;	/* list was empty */
    }
	qp->q_head = jp;

	pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Append a job on the tail of the queue.追加在队列尾
 */
void job_append(struct queue *qp, struct job *jp)
{
    assert(qp != NULL);
    assert(jp != NULL);

	pthread_rwlock_wrlock(&qp->q_lock);

	jp->j_next = NULL;
	jp->j_prev = qp->q_tail;
    if (qp->q_tail != NULL)
    {
        qp->q_tail->j_next = jp;
    }
    else
    {
        qp->q_head = jp;	/* list was empty */
    }
	qp->q_tail = jp;

	pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Remove the given job from a queue.
 */
void job_remove(struct queue *qp, struct job *jp)
{
    assert(qp != NULL);
    assert(jp != NULL);

	pthread_rwlock_wrlock(&qp->q_lock);

	if (jp == qp->q_head) 
    {
		qp->q_head = jp->j_next;

        if (qp->q_tail == jp)
        {
            qp->q_tail = NULL;
        }
	} 
    else if (jp == qp->q_tail) 
    {
		qp->q_tail = jp->j_prev;

        if (qp->q_head == jp)
        {
            qp->q_head = NULL;
        }
	} 
    else 
    {
		jp->j_prev->j_next = jp->j_next;
		jp->j_next->j_prev = jp->j_prev;
	}

	pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Find a job for the given thread ID.
 */
struct job *job_find(struct queue *qp, pthread_t id)
{
	struct job *jp = NULL;

    assert(qp != NULL);
    /*读锁*/
    if (pthread_rwlock_rdlock(&qp->q_lock) != 0)
    {
        return(NULL);
    }

    for (jp = qp->q_head; jp != NULL; jp = jp->j_next)
    {
        if (pthread_equal(jp->j_id, id))
        {
            break;
        }
    }
    
	pthread_rwlock_unlock(&qp->q_lock);
	return(jp);
}
/*
1.读写锁(共享-独占锁)：
1.1.更高的并行性；
1.2.非常适合于对数据结构读的次数远大于写的情况；

2.作业请求队列由单个读写锁保护，多个工作线程多取由单个主线程所分配的任务；

3.增、删作业--用写模式锁锁住队列读写锁，搜索队列--读模式下获取读写锁，可多线程并发搜索；
线程搜索队列频率远高于增删作业时，读写锁才能改善性能；

4.工作线程只能从队列读取与其线程ID匹配的作业，作业结构统一时间只能由一个线程使用，没必要加锁；
*/