/*8-13 ����system����
ʹ��system�����ǡ�fork+exec������system����������Ĵ������źŴ���*/

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
