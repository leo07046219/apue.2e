/*11-5 ʹ�û������������ݽṹ*/

#include <stdlib.h>
#include <pthread.h>

struct foo {
	int             f_count;
	pthread_mutex_t f_lock;
	/* ... more stuff here ... */
};

struct foo *foo_alloc(void)     /* allocate the object */
{
	struct foo *fp;

	if ((fp = malloc(sizeof(struct foo))) != NULL) 
    {
		fp->f_count = 1;
		if (pthread_mutex_init(&fp->f_lock, NULL) != 0) 
        {
			free(fp);
			return(NULL);
		}
		/* ... continue initialization ... */
	}
	return(fp);
}

void foo_hold(struct foo *fp)   /* add a reference to the object */
{
    assert(fp != NULL);

	pthread_mutex_lock(&fp->f_lock);
	fp->f_count++;
	pthread_mutex_unlock(&fp->f_lock);
}

void foo_rele(struct foo *fp)   /* release a reference to the object */
{
    assert(fp != NULL);

	pthread_mutex_lock(&fp->f_lock);
	if (0 == --fp->f_count)     /*�������ü�����Ҫ����*/
    { 
        /* last reference */
		pthread_mutex_unlock(&fp->f_lock);
		pthread_mutex_destroy(&fp->f_lock);
		free(fp);
	} 
    else 
    {
		pthread_mutex_unlock(&fp->f_lock);
	}
}
/*���������ڶ��ϣ����ü�������0��free*/