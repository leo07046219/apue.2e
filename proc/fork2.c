/*8-5 ����fork�����Ա��⽩������
forkһ���ӽ��̣�����Ҫ���ȴ��ӽ�����ֹ��Ҳ��ϣ���ӽ��̴��ڽ���״ֱ̬����������ֹ*/

#include "apue.h"
#include <sys/wait.h>

int main(void)
{
	pid_t	pid;

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (pid == 0) 
    {		
        /* first child */
        if ((pid = fork()) < 0)
        {
            err_sys("fork error");
        }
        else if (pid > 0)
        {
            exit(0);	/* parent from second fork == first child */
        }

		/*
		 * We're the second child; our parent becomes init as soon
		 * as our real parent calls exit() in the statement above.
		 * Here's where we'd continue executing, knowing that when
		 * we're done, init will reap our status.
         ����̣����ӽ���exit(0)ʱ����Ϊ�¶����̣���init�йܣ�
         �������ʱ��init���Զ�������̲������ض������γɽ�������
		 */
		sleep(2);/*��֤��ӡ������(init)idʱ����һ���ӽ����Ѿ���ֹ��*/
		printf("second child, parent pid = %d\n", getppid());

		exit(0);
	}

    if (waitpid(pid, NULL, 0) != pid)	/* wait for first child */
    {
        err_sys("waitpid error");
    }

	/*
	 * We're the parent (the original process); we continue executing,
	 * knowing that we're not the parent of the second child.
	 */

	exit(0);
}

/*
ubuntu
../apue.2e/proc$ ./fork2
../apue.2e/proc$ second child, parent pid = 1801
Y��
*/