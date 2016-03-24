/*10-7 具有超时限制的read调用*/

#include "apue.h"

static void	sig_alrm(int);

int main(void)
{
	int		n = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        err_sys("signal(SIGALRM) error");
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
	/* nothing to do, just return to interrupt the read */
}

/*
对可能阻塞的操作设置时间上限值，超时停止功能

问题：
1.alarm调用和read调用之间有竞争条件：内核在alarm调用和read调用之间使进程阻塞，
阻塞时间超过闹钟时间，read可能永远阻塞--一般超时时间设置较长，1分钟，该竞争发生概率较低
2.若系统调用是自动重启的，则当从SIGALRM信号处理程序返回时，read并不被中断。
此情况下设置时间限制不起作用。
*/
