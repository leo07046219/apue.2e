/*3-3 将标准输入复制到标准输出*/

#include "apue.h"

#define	BUFFSIZE	4096

int
main(void)
{
	int     n = 0;
	char    buf[BUFFSIZE];

    memset(buf, 0 ,sizeof(buf));

	while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0)
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
    
	exit(0);
}
