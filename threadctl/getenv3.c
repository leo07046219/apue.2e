/*12-5 线程安全的getenv的兼容版本*/

#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "apue.h"

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
pthread_mutex_t env_mutex = PTHREAD_MUTEX_INITIALIZER;

extern char **environ;

static void thread_init(void)
{
	pthread_key_create(&key, free);
}

char * getenv(const char *name)
{
	int		i, len;
	char	*envbuf;

    /*确保只为将要使用的线程私有数据创建一个键*/
	pthread_once(&init_done, thread_init);

	pthread_mutex_lock(&env_mutex);
    /*当无法修改应用程序以直接使用新接口时，
    可以用线程私有数据来维持每个线程的数据缓冲区的副本，用于存放各自的返回字符串*/
	envbuf = (char *)pthread_getspecific(key);
	if (NULL == envbuf) 
    {
        /*不是异步--信号安全*/
		envbuf = malloc(ARG_MAX);
		if (NULL == envbuf) 
        {
			pthread_mutex_unlock(&env_mutex);
			return(NULL);
		}
		pthread_setspecific(key, envbuf);
	}

	len = strlen(name);
	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) &&
		  (environ[i][len] == '=')) 
        {
			strcpy(envbuf, &environ[i][len+1]);
			pthread_mutex_unlock(&env_mutex);
			return(envbuf);
		}
	}
	pthread_mutex_unlock(&env_mutex);

	return(NULL);
}
/*利用“键”来自动释放malloc出来的线程私有数据，该数据被用于存放返回值
但所有的析构函数都调用完成后，系统会检查是否还有非null的线程私有数据值与键关联，
若有，再次调用析构函数，一直重复到线程所有的键都为null值线程私有数据，或达到最大尝试次数。*/