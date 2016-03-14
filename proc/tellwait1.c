/*8-6 ���о��������ĳ���*/

#include "apue.h"


static void charatatime(char *);

int main(void)
{
	pid_t	pid;

	if ((pid = fork()) < 0) 
    {
		err_sys("fork error");
	} 
    else if (0 == pid) 
    {
		charatatime("output from child\n");
	} 
    else 
    {
		charatatime("output from parent\n");
	}

	exit(0);
}

static void charatatime(char *pStr)
{
	char	*ptr = NULL;
	int		c = 0;

    assert(pStr != NULL);

    /* set unbuffered 
    ÿ���ַ��������Ҫ����һ��write�����ں˾����ܶ�����������̼��л�������ʾ��������*/
	setbuf(stdout, NULL);			

    for (ptr = pStr; (c = *ptr++) != 0; )
    {
        putc(c, stdout);
    }
}
