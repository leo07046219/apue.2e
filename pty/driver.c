/*19-7 pty程序的do_driver函数 使用-d选项时被pty的main调用*/

#include "apue.h"

void do_driver(char *driver)
{
	pid_t	child;
	int		pipe[2];

    memset(pipe, 0, sizeof(pipe));

	/*
	 * Create a stream pipe to communicate with the driver.
	 */
    if (s_pipe(pipe) < 0)
    {
        err_sys("can't create stream pipe");
    }

	if ((child = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == child) 
    {		
        /* child */
		close(pipe[1]);

		/* stdin for driver */
        if (dup2(pipe[0], STDIN_FILENO) != STDIN_FILENO)
        {
            err_sys("dup2 error to stdin");
        }
		/* stdout for driver */
        if (dup2(pipe[0], STDOUT_FILENO) != STDOUT_FILENO)
        {
            err_sys("dup2 error to stdout");
        }
        if (pipe[0] != STDIN_FILENO && pipe[0] != STDOUT_FILENO)
        {
            close(pipe[0]);
        }

		/* leave stderr for driver alone */
		execlp(driver, driver, (char *)0);
		err_sys("execlp error for: %s", driver);
	}

    /* parent */
	close(pipe[0]);		
    if (dup2(pipe[1], STDIN_FILENO) != STDIN_FILENO)
    {
        err_sys("dup2 error to stdin");
    }
    if (dup2(pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
    {
        err_sys("dup2 error to stdout");
    }
    if (pipe[1] != STDIN_FILENO && pipe[1] != STDOUT_FILENO)
    {
        close(pipe[1]);
    }

	/*
	 * Parent returns, but with stdin and stdout connected
	 * to the driver.
	 */
}
/*通过我们自己编写由pty调用的程序，可以按所希望的方式驱动交互式程序*/