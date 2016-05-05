/*17-19 客户进程main函数版本1--传文件路径给open服务器，从服务器获取打开文件的fd*/

#include	"open.h"
#include	<fcntl.h>

#define	BUFFSIZE	8192

int main(int argc, char *argv[])
{
	int		n = 0, fd = 0;
	char	buf[BUFFSIZE], line[MAXLINE];

    memset((char *)buf, 0, sizeof(buf));
    memset((char *)line, 0, sizeof(line));

	/* read filename to cat from stdin */
	while (fgets(line, MAXLINE, stdin) != NULL) 
    {
        if ('\n' == line[strlen(line) - 1])
        {
            line[strlen(line) - 1] = 0;     /* replace newline with null */
        }

		/* open the file 进程间打开，传路径给服务器，返回fd*/
        if ((fd = csopen(line, O_RDONLY)) < 0)
        {
            continue;	                    /* csopen() prints error from server */
        }

		/* and cat to stdout */
        while ((n = read(fd, buf, BUFFSIZE)) > 0)
        {
            if (write(STDOUT_FILENO, buf, n) != n)
            {
                err_sys("write error");
            }
        }

        if (n < 0)
        {
            err_sys("read error");
        }

		close(fd);
	}

	exit(0);
}
