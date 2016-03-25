/*10-8 使用longjmp，带超时限制调用read*/

#include "apue.h"
#include <setjmp.h>

static void		sig_alrm(int);
static jmp_buf	env_alrm;

int main(void)
{
	int		n = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        err_sys("signal(SIGALRM) error");
    }
    if (setjmp(env_alrm) != 0)
    {
        err_quit("read timeout");
    }

	alarm(10);

    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0)
    {
        err_sys("read error");
    }

	alarm(0);

	write(STDOUT_FILENO, line, n);

	exit(0);
}

static void sig_alrm(int signo)
{
	longjmp(env_alrm, 1);
}

/*
无需担心一个慢速系统调用是否被中断:
不管系统是否重新启动中断的系统调用，该程序会如所预期的工作，
但仍存在于其他信号处理程序交互的问题
*/
