/*8-16 产生会计数据的程序
1.会计记录写到指定文件，linux：/var/account/pacct
2.执行不带参数的accton可以停止会计处理
3.会计文件中记录的顺序对应于进程终止顺序，而非启动顺序
4.会计记录对应于进程，而非程序：一个进程顺序执行3个程序，A exec B,B exec C, C exit， 只会写一条会计记录
会计记录命名对应于C，但cpu时间是ABC之和*/

#include "apue.h"

int main(void)
{
	pid_t	pid;

    if ((pid = fork()) < 0)
    {
        err_sys("fork error");
    }
	else if (pid != 0) 
    {		
        /* parent */
		sleep(2);
		exit(2);				/* terminate with exit status 2 */
	}
							
    if ((pid = fork()) < 0)
    {
        err_sys("fork error");
    }
	else if (pid != 0) 
    {
        /* first child */
		sleep(4);
		abort();				/* terminate with core dump */
	}

								
    if ((pid = fork()) < 0)
    {
        err_sys("fork error");
    }
	else if (pid != 0) 
    {
        /* second child */
		execl("/bin/dd", "dd", "if=/etc/termcap", "of=/dev/null", NULL);
		exit(7);				/* shouldn't get here */
	}
								
    if ((pid = fork()) < 0)
    {
        err_sys("fork error");
    }
	else if (pid != 0) 
    {
        /* third child */
		sleep(8);
		exit(0);				/* normal exit */
	}

	/* fourth child */							
	sleep(6);
	kill(getpid(), SIGKILL);	/* terminate w/signal, no core dump */

	exit(6);					/* shouldn't get here */
}


/*
该程序调用fork4次，每个进程做不同的事情，然后终止*/