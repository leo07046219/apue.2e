/*8-2 vfork函数实例*/

#include "apue.h"

int		glob = 6;		        /* external variable in initialized data */

int
main(void)
{
	int		var = 0;		    /* automatic variable on the stack */
	pid_t	pid;

	var = 88;

	printf("before vfork\n");	/* we don't flush stdio */

	if ((pid = vfork()) < 0)    /*父进程不需要sleep等待子进程结束，vfork保证子进程先跑*/
    {
		err_sys("vfork error");
	} 
    else if (pid == 0) 
    {		
        /* child */
		glob++;					/* modify parent's variables */
		var++;
		_exit(0);				/* child terminates 不执行缓冲区冲洗，无输出*/
	}

	/*
	 * Parent continues here.
	 */
	printf("pid = %d, glob = %d, var = %d\n", getpid(), glob, var);

	exit(0);
}

/*
../apue.2e/proc$ ./vfork1
before vfork
pid = 29045, glob = 7, var = 89
*/
