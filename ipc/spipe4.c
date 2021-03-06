/*17-1 用STREAMS管道驱动add2过滤进程的程序*/

#include "apue.h"

static void	sig_pipe(int);      /* our signal handler */

int main(void)
{
	int		n = 0;
	int		fd[2];
	pid_t	pid;
	char	line[MAXLINE];

    memset(fd, 0, sizeof(fd));
    memset(line, 0, sizeof(line));

    if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
    {
        err_sys("signal error");
    }

    if (s_pipe(fd) < 0)         /* need only a single stream pipe */
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
		close(fd[1]);           /*父进程用fd[0]读写*/

		while (fgets(line, MAXLINE, stdin) != NULL) 
        {
			n = strlen(line);
            if (write(fd[0], line, n) != n)
            {
                err_sys("write error to pipe");
            }

            if ((n = read(fd[0], line, MAXLINE)) < 0)
            {
                err_sys("read error from pipe");
            }

			if (0 == n) 
            {
				err_msg("child closed pipe");
				break;
			}

			line[n] = 0;	    /* null terminate */

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
        close(fd[0]);           /*子进程用fd[1]读写*/

        if (fd[1] != STDIN_FILENO && \
            dup2(fd[1], STDIN_FILENO) != STDIN_FILENO)
        {
            err_sys("dup2 error to stdin");
        }

        if (fd[1] != STDOUT_FILENO && \
            dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
        {
            err_sys("dup2 error to stdout");
        }

        if (execl("./add2", "add2", (char *)0) < 0)
        {
            err_sys("execl error");
        }
	}

	exit(0);
}

static void sig_pipe(int signo)
{
	printf("SIGPIPE caught\n");
	exit(1);
}
/*仅利用了STREAM的全双工特性*/