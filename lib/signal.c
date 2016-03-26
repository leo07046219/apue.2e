
/*10-12 ��sigactionʵ�ֵ�signal����*/

#include "apue.h"

/* Reliable version of signal(), using POSIX sigaction().  */
Sigfunc * signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
    /*������sigemptyset��ʼ��act.sa_mask��Ա, act.sa_mask = 0 ���ܱ�֤��ͬ��Ч��*/
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (SIGALRM == signo)
    {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	} 
    else 
    {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}

    if (sigaction(signo, &act, &oact) < 0)
    {
        return(SIG_ERR);
    }

	return(oact.sa_handler);
}
