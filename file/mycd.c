/*4-8 chdir����ʵ��
chdirǰ��shell��ǰ����Ŀ¼���䣬shell�������ӽ���ִ��mycd*/

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
