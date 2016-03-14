/*8-6 具有竞争条件的程序*/

#include "apue.h"


static void charatatime(char *);

int main(void)
{
	pid_t	pid;

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == pid) 
    {
		charatatime("output from child\n");
	} 
    else 
    {
		charatatime("output from parent\n");
	}

	exit(0);
}

static void charatatime(char *pStr)
{
	char	*ptr = NULL;
	int		c = 0;

    assert(pStr != NULL);

    /* set unbuffered 
    每个字符输出都需要调用一次write，是内核尽可能多的在两个进程间切换，以演示竞争条件*/
	setbuf(stdout, NULL);			

    for (ptr = pStr; (c = *ptr++) != 0; )
    {
        putc(c, stdout);
    }
}
