/*15-6 将大写字符换成小写字符的过滤程序*/

#include "apue.h"
#include <ctype.h>

int main(void)
{
	int		c = 0;

	while ((c = getchar()) != EOF) 
    {
        if (isupper(c))
        {
            c = tolower(c);
        }
        if (putchar(c) == EOF)
        {
            err_sys("output error");
        }
        if ('\n' == c)
        {
            fflush(stdout);
        }
	}

	exit(0);
}
/*
将标准输入复制到标准输出，并大写转小写，行缓冲；
*/