/*14-1  长的非阻塞write*/

#include "apue.h"
#include <errno.h>
#include <fcntl.h>

#define MAX_BUF_LEN     500000

char	buf[MAX_BUF_LEN];

int main(void)
{
	int		ntowrite, nwrite;
	char	*ptr = NULL;

	ntowrite = read(STDIN_FILENO, buf, sizeof(buf));
	fprintf(stderr, "read %d bytes\n", ntowrite);

    /*设置标准输出为非阻塞*/
	set_fl(STDOUT_FILENO, O_NONBLOCK);	    /* set nonblocking */

	ptr = buf;
	while (ntowrite > 0) 
    {
		errno = 0;
		nwrite = write(STDOUT_FILENO, ptr, ntowrite);
		fprintf(stderr, "nwrite = %d, errno = %d\n", nwrite, errno);

		if (nwrite > 0) 
        {
			ptr += nwrite;
			ntowrite -= nwrite;
		}
	}

	clr_fl(STDOUT_FILENO, O_NONBLOCK);	    /* clear nonblocking */

	exit(0);
}
/*从标准输入读，写到标准输出上*/