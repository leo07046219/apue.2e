/*15-9 驱动add2过滤程序的程序*/

#include "apue.h"

static void	sig_pipe(int);		/* our signal handler */

int main(void)
{
	int		n, fd1[2], fd2[2];
	pid_t	pid;
	char	line[MAXLINE];

    memset(fd1, 0, sizeof(fd1));
    memset(fd2, 0, sizeof(fd2));
    memset(line, 0, sizeof(line));

    if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
    {
        err_sys("signal error");
    }

    if (pipe(fd1) < 0 || pipe(fd2) < 0)
    {
        err_sys("pipe error");
    }

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (pid > 0) 
    {							
        /* parent */
		close(fd1[0]);
		close(fd2[1]);

		while (fgets(line, MAXLINE, stdin) != NULL) 
        {
			n = strlen(line);
            if (write(fd1[1], line, n) != n)
            {
                err_sys("write error to pipe");
            }

			if ((n = read(fd2[0], line, MAXLINE)) < 0)
				err_sys("read error from pipe");

			if (0 == n) 
            {
				err_msg("child closed pipe");
				break;
			}

			line[n] = 0;                /* null terminate */
            if (fputs(line, stdout) == EOF)
            {
                err_sys("fputs error");
            }
		}

        if (ferror(stdin))
        {
            err_sys("fgets error on stdin");
        }

		exit(0);
	} 
    else 
    {									
        /* child */
		close(fd1[1]);
		close(fd2[0]);

        /*复制管道fd1输出为到标准输入*/
		if (fd1[0] != STDIN_FILENO) 
        {
            if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
            {
                err_sys("dup2 error to stdin");
            }
			close(fd1[0]);
		}
        /*复制管道fd2输出为到标准输出*/
		if (fd2[1] != STDOUT_FILENO) 
        {
            if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
            {
                err_sys("dup2 error to stdout");
            }
			close(fd2[1]);
		}

        if (execl("./add2", "add2", (char *)0) < 0)
        {
            err_sys("execl error");
        }
	}
	exit(0);
}

/*
若在程序等待输入时，若先杀死add2进程(如何实现？)，再输入2个数，则该管道以无读进程，调用信号处理程序
*/
static void sig_pipe(int signo)
{
	printf("SIGPIPE caught\n");
	exit(1);
}