/*10-4 sleep�ļ򵥶���������ʵ��*/

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
���⣺
1.sleep1()֮ǰ���Ѿ����������ӣ���֮ǰ����ʧЧ����Ҫ��֤֮ǰ�����Ӻ�Ԥ�ڵ�sleep1����Ч��
2.���������޸���alarm���ã����ܻᱻsleep1���ǣ�����sleep1֮ǰ�ȱ����ֳ���
3.��һ��alarm��pause���ھ�������æϵͳ�У�alarm�����ڵ���pauses֮ǰ��ʱ�����̱���Զ����:
����pause�����ߣ�alarm���ӵ��˲ű����ѣ����pause֮ǰ������������ִ�У������ߵĽ�����Զ���ᱻ���ѣ�
*/