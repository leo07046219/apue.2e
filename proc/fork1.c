/*8-1 fork函数实例*/

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

以交互方式运行，将标准输出重定向到文件后运行的区别

标准输出连接到终端设备，为行缓冲，否则为全缓冲

交互方式运行：标准输出缓冲区由换行符冲洗
标准输出重定向到文件：fork之前调用了printf一次，fork时，数据仍在缓冲区，
并被复制到子进程。父子进程各自有带该行内容的标准缓冲区。
进程终止时，冲洗缓冲区，输出
*/