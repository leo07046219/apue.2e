/*10-6 在一个捕捉其他信号的程序中调用sleep2*/

#include "apue.h"

unsigned int	sleep2(unsigned int);
static void		sig_int(int);

int main(void)
{
	unsigned int	unslept;

    if (signal(SIGINT, sig_int) == SIG_ERR)
    {
        err_sys("signal(SIGINT) error");
    }

	unslept = sleep2(5);

	printf("sleep2 returned: %u\n", unslept);

	exit(0);
}

static void sig_int(int signo)
{
	int				i, j;
	volatile int	k;/*阻止编译器丢弃循环语句*/

	/*
	 * Tune these loops to run for more than 5 seconds
	 * on whatever system this test program is run.
	 */
	printf("\nsig_int starting\n");

    for (i = 0; i < 300000; i++)
    {
        for (j = 0; j < 4000; j++)
        {
            k += i * j;
        }
    }

	printf("sig_int finished\n");
}
/*
SIGALRM中断了其他信号的处理程序，则到调用longjmp会提早终止该信号处理车工捏死
SIGINT处理程序中包含for语句，执行时间超过5秒

涉及信号时需要有精细而周到的考虑
*/