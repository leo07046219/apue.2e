/*1-5 �ӱ�׼��������ִ�С���shell�򻯰汾�������ܴ�shell����
fork+ exec = spawn*/

#include "apue.h"
#include <sys/wait.h>

int main(void)
{
	char	buf[MAXLINE];	/* from apue.h */
	pid_t	pid;
	int		status;

	printf("%% ");	/* print prompt (printf requires %% to print %) */

	while (fgets(buf, MAXLINE, stdin) != NULL) /*end when comes ctrl+D*/
    {
        /*execlpҪ��������null��β���滻fgetĩβ�Ļ��з�*/
        if (buf[strlen(buf) - 1] == '\n')
        {
            buf[strlen(buf) - 1] = 0; /* replace newline with null */
        }

		if ((pid = fork()) < 0) 
        {
			err_sys("fork error");
		} 
        else if (pid == 0) /* child */
        {		   
			execlp(buf, buf, (char *)0);
			err_ret("couldn't execute: %s", buf);
			exit(127);
		}

		if ((pid = waitpid(pid, &status, 0)) < 0)/* parent */
			err_sys("waitpid error");
		printf("%% ");
	}

	exit(0);
}
