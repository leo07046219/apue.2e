/*12-3 getenv的非可重入版本*/

#include <limits.h>
#include <string.h>
#include "apue.h"

/*∵所有调用getenv的线程返回的字符串都存放在同一静态缓冲区中，
∴不可重入*/
static char envbuf[ARG_MAX];

extern char **environ;

char *getenv(const char *name)
{
	int i = 0, len = 0;

	len = strlen(name);

	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) &&
		  (environ[i][len] == '=')) 
        {
			strcpy(envbuf, &environ[i][len+1]);
			return(envbuf);
		}
	}

	return(NULL);
}
/*
1.线程安全：
在同一时刻可以被多个线程安全地调用

2.不可重入的原因：
相关数据是保存在静态内存缓冲区的

3.运行时检查是否可重入
支持线程安全函数的os会在<unistd.h>中定义_POSIX_THREAD_SAFE_FUNCTIONS

4.可重入化  _r
将缓冲区改为自己提供的

5.可重入VS异步-信号安全
一个函数对多个线程是可重入的，则是线程安全的，并非说明对信号处理程序也是可重入的。
异步-信号安全：函数对异步信号处理程序的重入是安全的

*/