/*15-6 ����д�ַ�����Сд�ַ��Ĺ��˳���*/

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
����׼���븴�Ƶ���׼���������дתСд���л��壻
*/