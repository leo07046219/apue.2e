/*10-13 signal_intr函数，signal函数的另一个版本，力图阻止任何被中断的系统调用重新启动*/

#include "apue.h"

Sigfunc * signal_intr(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

#ifdef	SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif

    if (sigaction(signo, &act, &oact) < 0)
    {
        return(SIG_ERR);
    }

	return(oact.sa_handler);
}
