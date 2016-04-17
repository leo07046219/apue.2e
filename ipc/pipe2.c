/*15-2 将文件复制到分页程序*/

#include "apue.h"
#include <sys/wait.h>

#define	DEF_PAGER	"/bin/more"		/* default pPager program */

int main(int argc, char *argv[])
{
	int		n = 0;
	int		fd[2];
	pid_t	pid;
	char	*pPager = NULL, *pArgv0 = NULL;
	char	line[MAXLINE];
	FILE	*fp;

    memset(fd, 0, sizeof(fd));
    memset(line, 0, sizeof(line));

    if (argc != 2)
    {
        err_quit("usage: a.out <pathname>");
    }

	if ((fp = fopen(argv[1], "r")) == NULL)
    {
        err_sys("can't open %s", argv[1]);
    }

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
		close(fd[0]);		/* close read end */

		/* parent copies argv[1] to pipe */
		while (fgets(line, MAXLINE, fp) != NULL) 
        {
			n = strlen(line);
            if (write(fd[1], line, n) != n)
            {
                err_sys("write error to pipe");
            }
		}
        if (ferror(fp))
        {
            err_sys("fgets error");
        }

		close(fd[1]);           /* close write end of pipe for reader */

        if (waitpid(pid, NULL, 0) < 0) 
        {
            err_sys("waitpid error");
        }
        
		exit(0);
	} 
    else 
    {										
        /* child */
		close(fd[1]);           /* close write end */

        /*使子进程的标准输入成为管道的读端*/
		if (fd[0] != STDIN_FILENO) 
        {
            if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
            {
                err_sys("dup2 error to stdin");
            }

			close(fd[0]);       /* don't need this after dup2 */
		}

		/* get arguments for execl() 
        环境变量应用：使用环境变量获取用户分页程序名称*/
        if ((pPager = getenv("PAGER")) == NULL)
        {
            pPager = DEF_PAGER; /*获取失败，则使用系统默认值--套路*/
        }

        if ((pArgv0 = strrchr(pPager, '/')) != NULL)
        {
            pArgv0++;		    /* step past rightmost slash */
        }
        else
        {
            pArgv0 = pPager;    /* no slash in pPager */
        }

        if (execl(pPager, pArgv0, (char *)0) < 0)
        {
            err_sys("execl error for %s", pPager);
        }
	}
	exit(0);
}
/*
1.子进程的标准输入成为管道的读端，在子进程中exec分页程序，
然后分页程序的输入就被置为管道读端了？
*/