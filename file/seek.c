/*3-1 测试能否对标准输入设置偏移量*/
#include "apue.h"

int main(void)
{
    /*如果文件描述符引用的是一个管道、FIFO或者网络套接字，返回-1，errno被置为ESPIPE*/
    if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1)
    {
		printf("cannot seek\n");
    }
    else
    {
		printf("seek OK\n");
    }

	exit(0);
}
