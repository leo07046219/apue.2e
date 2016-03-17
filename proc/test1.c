/*8-16 ����������ݵĳ���
1.��Ƽ�¼д��ָ���ļ���linux��/var/account/pacct
2.ִ�в���������accton����ֹͣ��ƴ���
3.����ļ��м�¼��˳���Ӧ�ڽ�����ֹ˳�򣬶�������˳��
4.��Ƽ�¼��Ӧ�ڽ��̣����ǳ���һ������˳��ִ��3������A exec B,B exec C, C exit�� ֻ��дһ����Ƽ�¼
��Ƽ�¼������Ӧ��C����cpuʱ����ABC֮��*/

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
�ó������fork4�Σ�ÿ����������ͬ�����飬Ȼ����ֹ*/