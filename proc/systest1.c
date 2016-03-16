/*8-13 调用system函数
使用system，而非“fork+exec”，∵system进行了所需的错误处理及信号处理*/

#include "apue.h"
#include <sys/wait.h>

int main(void)
{
	int		status = 0;

    if ((status = system("date")) < 0)
    {
        err_sys("system() error");
    }
	pr_exit(status);

    if ((status = system("nosuchcommand")) < 0)
    {
        err_sys("system() error");
    }
	pr_exit(status);

    if ((status = system("who; exit 44")) < 0)
    {
        err_sys("system() error");
    }
	pr_exit(status);

	exit(0);
}
