/*15-12 在父、子进程间使用/dev/zero存储映射I/O的IPC*/

#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>

#define	NLOOPS		1000
#define	SIZE		sizeof(long)	/* size of shared memory area */

static int update(long *ptr)
{
	return((*ptr)++);	            /* return VALUE before increment */
}

int main(void)
{
    int	    fd = 0;
    int     i = 0; 
    int     counter = 0;
	pid_t	pid = 0;
	void	*pArea = NULL;

    if ((fd = open("/dev/zero", O_RDWR)) < 0)
    {
		err_sys("open error");
    }

    if ((pArea = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
		err_sys("mmap error");
    }

	close(fd);		                /* can close /dev/zero now that it's mapped 一旦映射成功即可关闭zero*/

	TELL_WAIT();

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (pid > 0) 
    {			
        /* parent */
		for (i = 0; i < NLOOPS; i += 2) 
        {
            if ((counter = update((long *)pArea)) != i)
            {
				err_quit("parent: expected %d, got %d", i, counter);
            }
			TELL_CHILD(pid);
			WAIT_CHILD();
		}
	} 
    else 
    {						
        /* child */
		for (i = 1; i < NLOOPS + 1; i += 2) 
        {
			WAIT_PARENT();

            if ((counter = update((long *)pArea)) != i)
            {
				err_quit("child: expected %d, got %d", i, counter);
            }

			TELL_PARENT(getppid());
		}
	}

	exit(0);
}
/*
1./dev/zero 0字节的无线资源，接收写向它的任何数据，但又忽略这些数据；

2.对此设备作为ipc，∵zero具有一些特殊的性质：
2.1.创建一个未名存储区，其长度是mmap的第二个参数，将其向上取整为系统的最近页长；
2.2.存储区都初始化为0；
2.3.如果多个进程的共同祖先进程对mmap指定了MAP_SHARED的标志，则这些进程可共享此存储区；

3.使用zero的
优点：在调用mmap创建营社区之前，无需存在一个实际文件；
缺点：只在相关进程间起作用
*/