/*8-5 调用fork两次以避免僵死进程
fork一个子进程，但不要它等待子进程终止，也不希望子进程处于僵死状态直到父进程终止*/

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
         孙进程，但子进程exit(0)时，成为孤儿进程，被init托管，
         当其结束时，init会自动清除进程残留，必定不会形成僵死进程
		 */
		sleep(2);/*保证打印父进程(init)id时，第一个子进程已经终止，*/
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
Y？
*/