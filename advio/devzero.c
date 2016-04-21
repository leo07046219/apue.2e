/*15-12 �ڸ����ӽ��̼�ʹ��/dev/zero�洢ӳ��I/O��IPC*/

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

	close(fd);		                /* can close /dev/zero now that it's mapped һ��ӳ��ɹ����ɹر�zero*/

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
1./dev/zero 0�ֽڵ�������Դ������д�������κ����ݣ����ֺ�����Щ���ݣ�

2.�Դ��豸��Ϊipc����zero����һЩ��������ʣ�
2.1.����һ��δ���洢�����䳤����mmap�ĵڶ�����������������ȡ��Ϊϵͳ�����ҳ����
2.2.�洢������ʼ��Ϊ0��
2.3.���������̵Ĺ�ͬ���Ƚ��̶�mmapָ����MAP_SHARED�ı�־������Щ���̿ɹ���˴洢����

3.ʹ��zero��
�ŵ㣺�ڵ���mmap����Ӫ����֮ǰ���������һ��ʵ���ļ���
ȱ�㣺ֻ����ؽ��̼�������
*/