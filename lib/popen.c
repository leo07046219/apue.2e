/*15-5 popen和pclose函数*/

#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
 * Pointer to array allocated at run-time.
 */
static pid_t	*childpid = NULL;

/*
 * From our open_max(), {Prog openmax}.
 */
static int		maxfd;

FILE * popen(const char *cmdstring, const char *type)
{
	int		i = 0;
	int		pfd[2];
	pid_t	pid;
	FILE	*fp = NULL;

    memset(pfd, 0, sizeof(pfd));

	/* only allow "r" or "w" */
	if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) 
    {
		errno = EINVAL;     /* required by POSIX */
		return(NULL);
	}

	if (NULL == childpid) 
    {		
        /* first time through */
		/* allocate zeroed out array for child pids */
		maxfd = open_max();
        /*一个进程可能调用多次popen，pid数组大小为maxfd*/
        if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
        {
            return(NULL);
        }
	}

    if (pipe(pfd) < 0)
    {
        return(NULL);       /* errno set by pipe() */
    }

	if ((pid = fork()) < 0) 
    {
		return(NULL);       /* errno set by fork() */
	} 
    else if (0 == pid) 
    {
        /* child */
		if ('r' == *type) 
        {
			close(pfd[0]);
			if (pfd[1] != STDOUT_FILENO) 
            {
				dup2(pfd[1], STDOUT_FILENO);
				close(pfd[1]);
			}
		} 
        else 
        {
			close(pfd[1]);
			if (pfd[0] != STDIN_FILENO) 
            {
				dup2(pfd[0], STDIN_FILENO);
				close(pfd[0]);
			}
		}

		/* close all descriptors in childpid[] */
        for (i = 0; i < maxfd; i++)
        {
            if (childpid[i] > 0)
            {
                close(i);
            }
        }

		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
		_exit(127);
	}

	/* parent continues... */
	if ('r' == *type) 
    {
		close(pfd[1]);
        if ((fp = fdopen(pfd[0], type)) == NULL)
        {
            return(NULL);
        }
	} 
    else 
    {
		close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
        {
            return(NULL);
        }
	}

	childpid[fileno(fp)] = pid;	/* remember child pid for this fd */
	return(fp);
}

int pclose(FILE *fp)
{
	int		fd, stat;
	pid_t	pid;

	if (NULL == childpid) 
    {
		errno = EINVAL;
		return(-1);		/* popen() has never been called */
	}

	fd = fileno(fp);
	if ((pid = childpid[fd]) == 0) 
    {
		errno = EINVAL;
		return(-1);     /* fp wasn't opened by popen() */
	}

	childpid[fd] = 0;
    if (fclose(fp) == EOF)
    {
        return(-1);
    }

    while (waitpid(pid, &stat, 0) < 0)
    {
        if (errno != EINTR)
        {
            return(-1); /* error other than EINTR from waitpid() */
        }
    }

	return(stat);       /* return child's termination status */
}
/*
1.popen决不应由设置用户ID或设置组ID程序调用，可以非法提升权限；
2.popen特别适用于构造简单的过滤器程序，变化运行命令的输入或输出。但命令希望构造它自己的管线时，就是此情形；
*/