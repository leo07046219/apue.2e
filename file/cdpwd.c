/*4-9 getcwd函数实例 cd 到一个/usr/spool/uucppublic目录，保存当前目录*/
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
    
    /*把当前目录的绝对地址保存到 ptr 中,ptr 的大小为 size。
    如果 size太小无法保存该地址，返回 NULL 并设置 errno 为 ERANGE*/

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
