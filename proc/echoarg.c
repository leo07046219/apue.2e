/*7-3 将所有命令行参数回送到标准输出*/

#include "apue.h"

int main(int argc, char *argv[])
{
	int		i = 0;
    /*亦可*/
    /*for (i = 0; argv[i] != NULL; i++)*/     
    for (i = 0; i < argc; i++)		        /* echo all command-line args */
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

	exit(0);
}
