/*11-7 简化的加、解锁*/

#include <stdlib.h>
#include <pthread.h>

#define NHASH 29
#define HASH(fp) (((unsigned long)fp)%NHASH)

struct foo *fh[NHASH];
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;

struct foo 
{
	int             f_count; /* protected by hashlock */
	pthread_mutex_t f_lock;
	struct foo     *f_next; /* protected by hashlock */
	int             f_id;
	/* ... more stuff here ... */
};

struct foo * foo_alloc(void) /* allocate the object */
{
	struct foo	*fp;
	int			idx;

	if ((fp = malloc(sizeof(struct foo))) != NULL) 
    {
		fp->f_count = 1;

		if (pthread_mutex_init(&fp->f_lock, NULL) != 0) 
        {
			free(fp);
			return(NULL);
		}

		idx = HASH(fp);
		pthread_mutex_lock(&hashlock);
		fp->f_next = fh[idx];
		fh[idx] = fp->f_next;
		pthread_mutex_lock(&fp->f_lock);
		pthread_mutex_unlock(&hashlock);

		/* ... continue initialization ... */
	}
	return(fp);
}

void foo_hold(struct foo *fp) /* add a reference to the object */
{
	pthread_mutex_lock(&hashlock);
	fp->f_count++;
	pthread_mutex_unlock(&hashlock);
}

struct foo * foo_find(int id) /* find a existing object */
{
	struct foo	*fp;
	int			idx;

	idx = HASH(fp);
	pthread_mutex_lock(&hashlock);
	for (fp = fh[idx]; fp != NULL; fp = fp->f_next) 
    {
		if (fp->f_id == id) 
        {
			fp->f_count++;
			break;
		}
	}
	pthread_mutex_unlock(&hashlock);
	return(fp);
}

void foo_rele(struct foo *fp) /* release a reference to the object */
{
	struct foo	*tfp;
	int			idx;

	pthread_mutex_lock(&hashlock);
	if (--fp->f_count == 0) 
    { /* last reference, remove from list */
		idx = HASH(fp);
		tfp = fh[idx];
		if (tfp == fp) 
        {
			fh[idx] = fp->f_next;
		} 
        else 
        {
            while (tfp->f_next != fp)
            {
                tfp = tfp->f_next;
            }

			tfp->f_next = fp->f_next;
		}
		pthread_mutex_unlock(&hashlock);
		pthread_mutex_destroy(&fp->f_lock);
		free(fp);
	} 
    else 
    {
		pthread_mutex_unlock(&hashlock);
	}
}
/*
1.用散列列表锁保护结构引用计数，使事情大大简化，结构互斥量可以用于保护foo结构中的其他任何东西；
2.多线程软件中经常需要考虑这类折中方案：
若锁太粗，就会出现很多线程阻塞等待相同的锁，源自并发性的改善微乎其微，
若锁太细，过多的锁开销会使系统性能受到影响，代码变得相当发杂，
在满足需求的情况下，找到代码复杂性和优化性能之间的平衡点；
*/