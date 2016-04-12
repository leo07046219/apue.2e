/*14-10 用getmsg将标准输入复制到标准输出*/

#include "apue.h"
#include <stropts.h>

#define	BUFFSIZE	4096

int main(void)
{
	int				n, flag;
	char			ctlbuf[BUFFSIZE], datbuf[BUFFSIZE];
	struct strbuf	ctl, dat;

    memset(ctlbuf, 0, sizeof(ctlbuf));
    memset(datbuf, 0, sizeof(datbuf));

	ctl.buf = ctlbuf;
	ctl.maxlen = BUFFSIZE;
	dat.buf = datbuf;
	dat.maxlen = BUFFSIZE;

	for ( ; ; ) 
    {
        /* return any message */
		flag = 0;		
        if ((n = getmsg(STDIN_FILENO, &ctl, &dat, &flag)) < 0)
        {
            err_sys("getmsg error");
        }

		fprintf(stderr, "flag = %d, ctl.len = %d, dat.len = %d\n", \
		  flag, ctl.len, dat.len);

        if (0 == dat.len)
        {
            exit(0);
        }
        else if (dat.len > 0)
        {
            if (write(STDOUT_FILENO, dat.buf, dat.len) != dat.len)
            {
                err_sys("write error");
            }
        }
	}
}
/*
apue.2e/advio$ echo hello, world! | ./catgetmsg
getmsg error: Function not implemented
*/