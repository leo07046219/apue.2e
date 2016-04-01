/*12-2 使用递归互斥量*/

#include "apue.h"
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

extern int makethread(void *(*)(void *), void *);

struct to_info 
{
	void	(*to_fn)(void *);	/* function */
	void	*to_arg;			/* argument */
	struct timespec to_wait;	/* time to wait */
};

#define SECTONSEC  1000000000	/* seconds to nanoseconds */
#define USECTONSEC 1000			/* microseconds to nanoseconds */

void * timeout_helper(void *arg)
{
	struct to_info	*tip;

	tip = (struct to_info *)arg;
	nanosleep(&tip->to_wait, NULL);

	(*tip->to_fn)(tip->to_arg);
	return(0);
}

void timeout(const struct timespec *when, void (*func)(void *), void *arg)
{
	struct timespec	now;
	struct timeval	tv;
	struct to_info	*tip;
	int				err;

	gettimeofday(&tv, NULL);
	now.tv_sec = tv.tv_sec;
	now.tv_nsec = tv.tv_usec * USECTONSEC;
	if ((when->tv_sec > now.tv_sec) ||
	  (when->tv_sec == now.tv_sec && when->tv_nsec > now.tv_nsec)) 
    {
		tip = malloc(sizeof(struct to_info));

		if (tip != NULL) 
        {
			tip->to_fn = func;
			tip->to_arg = arg;
			tip->to_wait.tv_sec = when->tv_sec - now.tv_sec;

			if (when->tv_nsec >= now.tv_nsec) 
            {
				tip->to_wait.tv_nsec = when->tv_nsec - now.tv_nsec;
			} 
            else 
            {
				tip->to_wait.tv_sec--;
				tip->to_wait.tv_nsec = SECTONSEC - now.tv_nsec +
				  when->tv_nsec;
			}
			err = makethread(timeout_helper, (void *)tip);
            if (0 == err)
            {
                return;
            }
		}
	}

	/*
	 * We get here if (a) when <= now, or (b) malloc fails, or
	 * (c) we can't make a thread, so we just call the function now.
	 */
	(*func)(arg);
}

pthread_mutexattr_t attr;
pthread_mutex_t mutex;

void retry(void *arg)
{
	pthread_mutex_lock(&mutex);
	/* perform retry steps ... */

	pthread_mutex_unlock(&mutex);
}

int main(void)
{
	int     err, condition, arg;
	struct  timespec	when;

    if ((err = pthread_mutexattr_init(&attr)) != 0)
    {
        err_exit(err, "pthread_mutexattr_init failed");
    }

    if ((err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) != 0)
    {
        err_exit(err, "can't set recursive type");
    }

    if ((err = pthread_mutex_init(&mutex, &attr)) != 0)
    {
        err_exit(err, "can't create recursive mutex");
    }
	/* ... */

	pthread_mutex_lock(&mutex);
	/* ... */

	if (condition) 
    {
		/* calculate target time "when" */
		timeout(&when, retry, (void *)arg);
	}
	/* ... */

	pthread_mutex_unlock(&mutex);
	/* ... */

	exit(0);
}
/*
1.使用递归互斥量并不是好方法--改变条件时必须占有互斥量，有潜在的“死锁”风险；
2.入股而需要把现有的单线程接口放到多线程环境中，递归互斥量非常有用，但由于程序兼容性的限制，
不能对函数接口进行更改；
3.递归锁的使用需要一定技巧，它只应在没有其他可行方案的情况下使用；*/
