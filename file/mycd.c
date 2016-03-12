/*4-8 chdir函数实例
chdir前后shell当前工作目录不变，shell创建了子进程执行mycd*/

#include "apue.h"

int main(void)
{
    if (chdir("/tmp") < 0)
    {
		err_sys("chdir failed");
    }

	printf("chdir to /tmp succeeded\n");
	exit(0);
}
