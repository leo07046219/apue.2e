/*10-20 system函数的POSIX.1正确实现*/

#include	<sys/wait.h>
#include	<errno.h>
#include	<signal.h>
#include	<unistd.h>

int system(const char *cmdstring)	/* with appropriate signal handling */
{
	pid_t				pid;
	int					status;
	struct sigaction	ignore, saveintr, savequit;
	sigset_t			chldmask, savemask;

    if (NULL == cmdstring)
    {
        return(1);		            /* always a command processor with UNIX */
    }

	ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
	sigemptyset(&ignore.sa_mask);
	ignore.sa_flags = 0;

    if (sigaction(SIGINT, &ignore, &saveintr) < 0)
    {
        return(-1);
    }
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
    {
        return(-1);
    }

	sigemptyset(&chldmask);			/* now block SIGCHLD */
	sigaddset(&chldmask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
    {
        return(-1);
    }

	if ((pid = fork()) < 0) 
    {
		status = -1;	            /* probably out of processes */
	} 
    else if (0 == pid) 
    {			
        /* child */
		/* restore previous signal actions & reset signal mask */
		sigaction(SIGINT, &saveintr, NULL);
		sigaction(SIGQUIT, &savequit, NULL);
		sigprocmask(SIG_SETMASK, &savemask, NULL);

		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
		_exit(127);		            /* exec error */
	} 
    else 
    {						
        /* parent */
        while (waitpid(pid, &status, 0) < 0)
        {
			if (errno != EINTR) 
            {
				status = -1; /* error other than EINTR from waitpid() */
				break;
			}
        }

	}

	/* restore previous signal actions & reset signal mask */
    if (sigaction(SIGINT, &saveintr, NULL) < 0)
    {
        return(-1);
    }
    if (sigaction(SIGQUIT, &savequit, NULL) < 0)
    {
        return(-1);
    }
    if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
    {
        return(-1);
    }

	return(status);
}
/*
1.中断或退出字符不向调用进程发送信号；
2.不向调用进程发送SIGCHLD信号；
3.system的返回值，是shell的终止状态，但并不总是执行命令字符串进程的终止状态，
仅当shell本身异常终止时，system的返回值才报告一个异常终止；
*/