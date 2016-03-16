/*8-15 打印实际和有效用户id*/

#include "apue.h"

int main(void)
{
	printf("real uid = %d, effective uid = %d\n", getuid(), geteuid());
	exit(0);
}
