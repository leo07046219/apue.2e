/*10-14 信号屏蔽字、sigsetjmp和siglongjmp实例*/

#include "apue.h"
#include <setjmp.h>
#include <time.h>

static void						sig_usr1(int), sig_alrm(int);
static sigjmp_buf				jmpbuf;
/*
sig_atomic_t
由ISO C标准定义的变量类型，在写这种类型的变量时不会被中断。
在具有虚拟存储器的系统上这种变量不会跨越页边界，可以用一条机器指令访问。
这种类型的变量总是包括ISO类型修饰符volatile，
其原因是：该变量将两个不同的控制线程--main函数和异步执行的信号处理程序访问
*/
static volatile sig_atomic_t	canjump;

int main(void)
{
    if (signal(SIGUSR1, sig_usr1) == SIG_ERR)
    {
        err_sys("signal(SIGUSR1) error");
    }

    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        err_sys("signal(SIGALRM) error");
    }
	pr_mask("starting main: ");		/* {Prog prmask} */

	if (sigsetjmp(jmpbuf, 1)) 
    {
		pr_mask("ending main: ");
		exit(0);
	}

	canjump = 1;	/* now sigsetjmp() is OK */

    for (; ; )
    {
        pause();
    }
}

static void sig_usr1(int signo)
{
	time_t	starttime;
    /*，只有当canjump标志为1时(sigsetjmp已经被调用过)，才调用信号处理程序*/
    if (0 == canjump)
    {
        return;		/* unexpected signal, ignore */
    }

	pr_mask("starting sig_usr1: ");
	alarm(3);				/* SIGALRM in 3 seconds */
	starttime = time(NULL);

    for (; ; )				/* busy wait for 5 seconds */
    {
        if (time(NULL) > starttime + 5)
        {
            break;
        }
    }

	pr_mask("finishing sig_usr1: ");

	canjump = 0;
	siglongjmp(jmpbuf, 1);	/* jump back to main, don't return */
}

static void sig_alrm(int signo)
{
	pr_mask("in sig_alrm: ");
}

/*
1.该实例演示了在信号处理程序被调用时，系统所设置的信号屏蔽字如何自动地包括刚被捕捉到的信号，
也通过实例说明了如何使用sigsetjmp和siglongjmp函数。

*/