/*18-9 µ÷ÓÃgetpassº¯Êý*/

#include "apue.h"

char	*getpass(const char *);

int main(void)
{
	char	*ptr = NULL;

    if (NULL == (ptr = getpass("Enter password:")))
    {
        err_sys("getpass error");
    }
	printf("password: %s\n", ptr);

	/* now use password (probably encrypt it) ... */

    while (*ptr != 0)
    {
        *ptr++ = 0;		/* zero it out when we're done with it */
    }
	exit(0);
}
