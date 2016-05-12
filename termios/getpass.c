/*18-8 getpass函数的实现 -- 禁用回显*/

#include	<signal.h>
#include	<stdio.h>
#include	<termios.h>

#define	MAX_PASS_LEN	8		/* max #chars for user to enter */

char *getpass(const char *prompt)
{
	static char		buf[MAX_PASS_LEN + 1];	/* null byte at end */
	char			*ptr;
	sigset_t		sig, osig;
	struct termios	ts, ots;
	FILE			*fp;
	int				c;

    /*打开控制终端，而非直接打开硬编码的/dev/tty*/
    if (NULL == (fp = fopen(ctermid(NULL), "r+")))
    {
        return(NULL);
    }
	setbuf(fp, NULL);

    /*阻塞2个信号，防止INTR/SUSP后终端禁止回显*/
	sigemptyset(&sig);
	sigaddset(&sig, SIGINT);		/* block SIGINT */
	sigaddset(&sig, SIGTSTP);		/* block SIGTSTP */
	sigprocmask(SIG_BLOCK, &sig, &osig);	/* and save mask */

	tcgetattr(fileno(fp), &ts);		/* save tty state */
	ots = ts;						/* structure copy */
	ts.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);/*禁用回显*/
	tcsetattr(fileno(fp), TCSAFLUSH, &ts);
	fputs(prompt, fp);

	ptr = buf;
    while ((c = getc(fp)) != EOF && (c != '\n'))
    {
        if (ptr < &buf[MAX_PASS_LEN])
        {
            *ptr++ = c;
        }
    }
	*ptr = 0;			/* null terminate */
	putc('\n', fp);		/* we echo a newline */

	tcsetattr(fileno(fp), TCSAFLUSH, &ots); /* restore TTY state */
	sigprocmask(SIG_SETMASK, &osig, NULL);  /* restore mask */
	fclose(fp);			/* done with /dev/tty */
	
    return(buf);
}
