/*15-1 经由管道，从父进程想子进程传送数据*/

#include "apue.h"

int main(void)
{
	int		n = 0;
    int		fd[2] = {0};
	pid_t	pid;
	char	line[MAXLINE];

    memset(fd, 0, sizeof(fd));
    memset(line, 0, sizeof(line));

    if (pipe(fd) < 0)
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
		close(fd[0]);
		write(fd[1], "hello world\n", 12);
	} 
    else 
    {				
        /* child */
		close(fd[1]);
		n = read(fd[0], line, MAXLINE);
		write(STDOUT_FILENO, line, n);
	}

	exit(0);
}
/*
1.fd[0]--read,fd[1]--write

*/