/*8-15 ��ӡʵ�ʺ���Ч�û�id*/

#include "apue.h"

int main(void)
{
	printf("real uid = %d, effective uid = %d\n", getuid(), geteuid());
	exit(0);
}
