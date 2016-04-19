/*15-10 ��2������͵��˲����� ʹ�ñ�׼I/O*/

#include "apue.h"

int main(void)
{
	int		int1 = 0, int2 = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

#if 1
    /*�޸�Эͬ���̻�������Ϊ�л��壬������������*/
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
15-9���ô˳��򣬹����쳣��
ϵͳĬ�ϵ�stdio�������Ϊȫ���壬
add2stdio�ӱ�׼�����ʱ����������15-9�ӹܵ���ʱҲ����������������������
*/