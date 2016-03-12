/*4-9 getcwd����ʵ�� cd ��һ��/usr/spool/uucppublicĿ¼�����浱ǰĿ¼*/
#include "apue.h"

int main(void)
{
	char *ptr = NULL;
	int  size;

    if (chdir("/usr/spool/uucppublic") < 0)
    {
		err_sys("chdir failed");
    }

	ptr = path_alloc(&size);	/* our own function */
    
    /*�ѵ�ǰĿ¼�ľ��Ե�ַ���浽 ptr ��,ptr �Ĵ�СΪ size��
    ��� size̫С�޷�����õ�ַ������ NULL ������ errno Ϊ ERANGE*/

    if (ptr != NULL)
    {
        if (getcwd(ptr, size) == NULL)
        {
            err_sys("getcwd failed");
        }
        	
        printf("cwd = %s\n", ptr);        
    }
    
	exit(0);
}
