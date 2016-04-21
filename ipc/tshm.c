/*15-11 打印各种不同类型的数据所存放的位置*/

#include "apue.h"
#include <sys/shm.h>

#define	ARRAY_SIZE	40000
#define	MALLOC_SIZE	100000
#define	SHM_SIZE	100000
#define	SHM_MODE	0600	/* user read/write */

char	array[ARRAY_SIZE];	/* uninitialized data = bss */

int main(void)
{
	int		shmid = 0;
	char	*pBuf = NULL, *pShareMem = NULL;

	printf("array[] from %lx to %lx\n", (unsigned long)&array[0], \
	  (unsigned long)&array[ARRAY_SIZE]);
	printf("stack around %lx\n", (unsigned long)&shmid);

    if ((pBuf = malloc(MALLOC_SIZE)) == NULL)
    {
        err_sys("malloc error");
    }
	printf("malloced from %lx to %lx\n", (unsigned long)pBuf, \
	  (unsigned long)pBuf+MALLOC_SIZE);

    if ((shmid = shmget(IPC_PRIVATE, SHM_SIZE, SHM_MODE)) < 0)
    {
        err_sys("shmget error");
    }
    if ((pShareMem = shmat(shmid, 0, 0)) == (void *)-1)
    {
        err_sys("shmat error");
    }
	printf("shared memory attached from %lx to %lx\n", \
	  (unsigned long)pShareMem, (unsigned long)pShareMem+SHM_SIZE);

    if (shmctl(shmid, IPC_RMID, 0) < 0)
    {
        err_sys("shmctl error");
    }

	exit(0);
}
/*
1.内核将以地址0连接的共享存储段放在什么位置上与系统密切相关。
2.mmap和shm区别：mmap映射的存储段是与文件关联，shm共享存储段并无这种关联
*/