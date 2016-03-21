/*10-1 捕捉SIGUSR1和SIGUSR2的简单程序*/

#include "apue.h"

static void	sig_usr(int);	/* one handler for both signals */

int main(void)
{
    if (signal(SIGUSR1, sig_usr) == SIG_ERR)
    {
        err_sys("can't catch SIGUSR1");
    }

    if (signal(SIGUSR2, sig_usr) == SIG_ERR)
    {
        err_sys("can't catch SIGUSR2");
    }

    for (; ; )
    {
        pause();
    }
}

static void sig_usr(int signo)		/* argument is signal number */
{
    if (SIGUSR1 == signo)
    {
        printf("received SIGUSR1\n");
    }
    else if (SIGUSR2 == signo)
    {
        printf("received SIGUSR2\n");
    }
    else
    {
        err_dump("received signal %d\n", signo);
    }
}
/*
apue.2e/signals$ kill -USR1 79410
received SIGUSR1
apue.2e/signals$ kill -USR2 79410
received SIGUSR2
apue.2e/signals$ kill 79410
[3]   已终止               ./sigusr

*/
