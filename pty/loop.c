/*19-6 loop函数*/

#include "apue.h"

#define	BUFFSIZE	512

static void	sig_term(int);
static volatile sig_atomic_t	sigcaught;	/* set by signal handler */

void loop(int ptym, int ignoreeof)
{
	pid_t	child;
	int		nread = 0;
	char	buf[BUFFSIZE];

    memset(buf, 0, sizeof(buf));

	if ((child = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == child) 
    {	
        /* child copies stdin to ptym */
		for ( ; ; ) 
        {
            if ((nread = read(STDIN_FILENO, buf, BUFFSIZE)) < 0)
            {
                err_sys("read error from stdin");
            }
            else if (0 == nread)
            {
                break;		/* EOF on stdin means we're done */
            }
            if (writen(ptym, buf, nread) != nread)
            {
                err_sys("writen error to master pty");
            }
		}

		/*
		 * We always terminate when we encounter an EOF on stdin,
		 * but we notify the parent only if ignoreeof is 0.
		 */
        if (0 == ignoreeof)
        {
            kill(getppid(), SIGTERM);	/* notify parent */
        }

		exit(0);	/* and terminate; child can't return */
	}

	/*
	 * Parent copies ptym to stdout.
	 */
    if (signal_intr(SIGTERM, sig_term) == SIG_ERR)
    {
        err_sys("signal_intr error for SIGTERM");
    }

	for ( ; ; ) 
    {
        if ((nread = read(ptym, buf, BUFFSIZE)) <= 0)
        {
            break;		/* signal caught, error, or EOF */
        }
        if (writen(STDOUT_FILENO, buf, nread) != nread)
        {
            err_sys("writen error to stdout");
        }
	}

	/*
	 * There are three ways to get here: sig_term() below caught the
	 * SIGTERM from the child, we read an EOF on the pty master (which
	 * means we have to signal the child to stop), or an error.
	 */
    if (0 == sigcaught)	/* tell child if it didn't send us the signal */
    {
        kill(child, SIGTERM);
    }
	/*
	 * Parent returns to caller.
	 */
}

/*
 * The child sends us SIGTERM when it gets EOF on the pty slave or
 * when read() fails.  We probably interrupted the read() of ptym.
 */
static void sig_term(int signo)
{
	sigcaught = 1;		/* just set flag and return */
}
/*
使用2个进程时，如果一个终止，那么它必须通知另一个，此处用SIGTERM信号实现
*/