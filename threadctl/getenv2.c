/*12-4 getenv的可重入(线程安全)版本*/

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/*调用者自己提供缓冲区，避免其他线程的干扰*/
extern char **environ;

pthread_mutex_t env_mutex;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;

static void thread_init(void)
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
    
    /*若使用的是非递归互斥量，但线程从信号处理程序中调用getenv_r时，就有可能出现死锁
    如果信号处理程序在线程执行getenv_r时中断了该线程，由于这是已经占有加锁的env_mutex,
    这样其他线程试图对这个互斥量的加锁过程就会被阻塞，最终导致线程进入死锁状态。

    ∴必须使用递归互斥量阻止其他线程改变当前正在查看的数据结构，

    同时，还要阻止来自信号处理程序的死锁，由于pthread函数并非保证信号安全，所以不能把它用于其他函数(?)*/

	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&env_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

int getenv_r(const char *name, char *buf, int buflen)
{
	int i, len, olen;

	pthread_once(&init_done, thread_init);
	len = strlen(name);

    /*互斥锁保证在搜索请求的字符串时环境不被修改，对环境列表的访问序列化*/
	pthread_mutex_lock(&env_mutex);

	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) && \
		  (environ[i][len] == '=')) 
        {
			olen = strlen(&environ[i][len+1]);
			if (olen >= buflen) 
            {
				pthread_mutex_unlock(&env_mutex);
				return(ENOSPC);
			}
			strcpy(buf, &environ[i][len+1]);
			pthread_mutex_unlock(&env_mutex);
			return(0);
		}
	}

	pthread_mutex_unlock(&env_mutex);

	return(ENOENT);
}