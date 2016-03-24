/*10-4 sleep的简单而不完整的实现*/

#include	<signal.h>
#include	<unistd.h>

static void sig_alrm(int signo)
{
	/* nothing to do, just return to wake up the pause */
}

unsigned int sleep1(unsigned int nsecs)
{
    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        return(nsecs);
    }

	alarm(nsecs);		/* start the timer */
	pause();			/* next caught signal wakes us up */
	return(alarm(0));	/* turn off timer, return unslept time */
}
/*
问题：
1.sleep1()之前若已经设置了闹钟，则之前闹钟失效，需要保证之前的闹钟和预期的sleep1都有效；
2.其他程序修改了alarm配置，可能会被sleep1覆盖，配置sleep1之前先保存现场；
3.第一次alarm和pause存在竞争：繁忙系统中，alarm可能在调用pauses之前超时，进程被永远挂起:
进程pause后休眠，alarm闹钟到了才被唤醒，如果pause之前，唤醒条件已执行，则休眠的进程永远不会被唤醒；
*/