/*7-3 �����������в������͵���׼���*/

#include "apue.h"

int main(int argc, char *argv[])
{
	int		i = 0;
    /*���*/
    /*for (i = 0; argv[i] != NULL; i++)*/     
    for (i = 0; i < argc; i++)		        /* echo all command-line args */
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

	exit(0);
}
