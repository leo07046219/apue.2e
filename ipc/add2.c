/*15-8 对两个数求和的简单过滤程序*/

#include "apue.h"

int main(void)
{
	int		n = 0, int1 = 0, int2 = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

	while ((n = read(STDIN_FILENO, line, MAXLINE)) > 0) 
    {
		line[n] = 0;		/* null terminate */

		if (sscanf(line, "%d%d", &int1, &int2) == 2) 
        {
			sprintf(line, "%d\n", int1 + int2);
			n = strlen(line);

            if (write(STDOUT_FILENO, line, n) != n)
            {
                err_sys("write error");
            }
		} 
        else 
        {
            if (write(STDOUT_FILENO, "invalid args\n", 13) != 13)
            {
                err_sys("write error");
            }
		}
	}

	exit(0);
}
/*
从标准输入读2个数，求和，写到标准输出
*/