/*1-3 �ñ�׼IO����׼���븴�Ƶ���׼���*/
/*5-1 ��getc��putc����׼���븴�Ƶ���׼���*/

#include "apue.h"

int main(void)
{
	int		c;

	while ((c = getc(stdin)) != EOF)
	{
        if (putc(c, stdout) == EOF)
        {
            err_sys("output error");
        }

     }

	if (ferror(stdin))
	{
		err_sys("input error");        
     }

	exit(0);
}
