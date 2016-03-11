/*8-1 fork����ʵ��*/

#include "apue.h"

int		glob = 6;		/* external variable in initialized data */
char	buf[] = "a write to stdout\n";

int main(void)
{
	int		var = 0;		/* automatic variable on the stack */
	pid_t	pid;

	var = 88;
    if (write(STDOUT_FILENO, buf, sizeof(buf) - 1) != sizeof(buf) - 1)
    {
        err_sys("write error");
    }

	printf("before fork\n");	/* we don't flush stdout */

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == pid) 
    {		                    /* child */
		glob++;					/* modify variables */
		var++;
	} 
    else 
    {
		sleep(2);				/* parent */
	}

	printf("pid = %d, glob = %d, var = %d\n", getpid(), glob, var);

	exit(0);
}


/*
../apue.2e/proc$ ./fork1
a write to stdout
before fork
pid = 27425, glob = 7, var = 89
pid = 27424, glob = 6, var = 88

../apue.2e/proc$ ./fork1 > fork1.out
../apue.2e/proc$ cat fork1.out
a write to stdout
before fork
pid = 27444, glob = 7, var = 89
before fork
pid = 27443, glob = 6, var = 88

�Խ�����ʽ���У�����׼����ض����ļ������е�����

��׼������ӵ��ն��豸��Ϊ�л��壬����Ϊȫ����

������ʽ���У���׼����������ɻ��з���ϴ
��׼����ض����ļ���fork֮ǰ������printfһ�Σ�forkʱ���������ڻ�������
�������Ƶ��ӽ��̡����ӽ��̸����д��������ݵı�׼��������
������ֹʱ����ϴ�����������
*/