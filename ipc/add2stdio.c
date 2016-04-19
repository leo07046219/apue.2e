/*15-10 对2个数求和的滤波程序， 使用标准I/O*/

#include "apue.h"

int main(void)
{
	int		int1 = 0, int2 = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

#if 1
    /*修改协同进程缓冲类型为行缓冲，程序正常工作*/
    if (setvbuf(stdin, NULL, _IOLBF, 0) != 0)
    {
        err_sys("setVbuf error");
    }
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0)
    {
        err_sys("setVbuf error");
    }
#endif

	while (fgets(line, MAXLINE, stdin) != NULL) 
    {
		if (sscanf(line, "%d%d", &int1, &int2) == 2) 
        {
            if (printf("%d\n", int1 + int2) == EOF)
            {
                err_sys("printf error");
            }
		} 
        else 
        {
            if (printf("invalid args\n") == EOF)
            {
                err_sys("printf error");
            }
		}
	}
	exit(0);
}
/*
15-9调用此程序，工作异常。
系统默认的stdio缓冲机制为全缓冲，
add2stdio从标准输入读时发生阻塞，15-9从管道读时也发生阻塞，，导致死锁；
*/