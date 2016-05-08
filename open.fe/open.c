/*17-20 csopen函数版本1--主进程客户端程序，子进程：服务器程序，向open服务器发送文件名，获取fd*/

#include	"open.h"
#include	<sys/uio.h>		/* struct iovec */

/*
 * Open the file by sending the "name" and "oflag" to the
 * connection server and reading a file descriptor back.
 */
int csopen(char *name, int oflag)
{
	pid_t			pid;
	int				len = 0;
	char			buf[10];
	struct iovec	iov[3];
	static int		fd[2] = { -1, -1 };

    memset((char *)buf, 0, sizeof(buf));
    memset((char *)iov, 0, sizeof(iov));

	if (fd[0] < 0) 
    {	
        /* fork/exec our open server first time */
        if (s_pipe(fd) < 0)
        {
            err_sys("s_pipe error");
        }

		if ((pid = fork()) < 0) 
        {
			err_sys("fork error");
		} 
        else if (0 == pid) 
        {		
            /* child */
			close(fd[0]);

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

            if (execl("../opend.fe/opend", "opend", (char *)0) < 0)
            {
                err_sys("execl error");
            }
		}
        /* parent */
		close(fd[1]);				                
	}

    /*父进程传送路径名和打开模式给子进程*/
	sprintf(buf, " %d", oflag);		                /* oflag to ascii */
	iov[0].iov_base = CL_OPEN " ";		            /* string concatenation */
	iov[0].iov_len  = strlen(CL_OPEN) + 1;
	iov[1].iov_base = name;
	iov[1].iov_len  = strlen(name);
	iov[2].iov_base = buf;
	iov[2].iov_len  = strlen(buf) + 1;	            /* +1 for null at end of buf */
	len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;

    if (writev(fd[0], &iov[0], 3) != len)
    {
        err_sys("writev error");
    }

	/* read descriptor, returned errors handled by write() 出错，write出错消息*/
	return(recv_fd(fd[0], write));
}
