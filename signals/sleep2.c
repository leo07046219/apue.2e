/*10-5 sleep的另一个(不完善)实现*/

#include	<setjmp.h>
#include	<signal.h>
#include	<unistd.h>

static jmp_buf	env_alrm;

static void sig_alrm(int signo)
{
	longjmp(env_alrm, 1);
}

unsigned int sleep2(unsigned int nsecs)
{
    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        return(nsecs);
    }

	if (setjmp(env_alrm) == 0) 
    {
		alarm(nsecs);		/* start the timer */
		pause();			/* next caught signal wakes us up */
	}

	return(alarm(0));		/* turn off timer, return unslept time */
}
/*
避免了sleep1.c中的竞争条件，保证了闹钟在pause之前被触发
问题：SIGALARM中断了某个其他信号处理程序，则调用longjmp会提早终止该信号处理程序
*/