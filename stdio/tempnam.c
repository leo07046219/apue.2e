/*5-5 演示tempnam函数：允许 调用者为所产生的路径名指定目录和前缀*/

#include "apue.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        err_quit("usage: a.out <directory> <prefix>");
    }

	printf("%s\n", tempnam(argv[1][0] != ' ' ? argv[1] : NULL, \
	  argv[2][0] != ' ' ? argv[2] : NULL));

	exit(0);
}
