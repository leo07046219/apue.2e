/*8-12 system函数(无信号处理)*/

#include	<sys/wait.h>
#include	<errno.h>
#include	<unistd.h>

int system(const char *cmdstring)	/* version without signal handling */
{
	pid_t	pid;
	int		status = 0;

    if (NULL == cmdstring)
    {
        return(1);		        /* always a command processor with UNIX */
    }

	if ((pid = fork()) < 0) 
    {
		status = -1;	        /* probably out of processes */
	} 
    else if (0 == pid) 
    {				
        /* child */
        /*-c 告诉shell程序取下一个命令行参数作为命令输入*/
		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        /*不用exit，是为了防止任意标准IO缓冲区在子进程中被冲洗*/
		_exit(127);		        /* execl error */
	} 
    else 
    {							
        /* parent */
		while (waitpid(pid, &status, 0) < 0) 
        {
			if (errno != EINTR) 
            {
				status = -1; /* error other than EINTR from waitpid() */
				break;
			}
		}
	}

	return(status);
}
