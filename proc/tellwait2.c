/*8-7 修改程序8-6以避免竞争条件*/

#include "apue.h"

static void charatatime(char *);

int main(void)
{
	pid_t	pid;

	TELL_WAIT();

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == pid) 
    {        
		/*WAIT_PARENT();*/		            /* parent goes first */
		charatatime("output from child\n"); /* child goes first */
        TELL_PARENT(pid);
	} 
    else 
    {
        WAIT_CHILD();		   
		charatatime("output from parent\n");
		/*TELL_CHILD(pid);*/
	}

	exit(0);
}

static void charatatime(char *pStr)
{
	char	*ptr;
	int		c = 0;

    assert(pStr != NULL);

	setbuf(stdout, NULL);			/* set unbuffered */
    for (ptr = pStr; (c = *ptr++) != 0; )
    {
        putc(c, stdout);
    }
}
