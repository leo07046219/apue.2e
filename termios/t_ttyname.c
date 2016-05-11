/*18-7 ²âÊÔttynameº¯Êý*/

#include "apue.h"

int main(void)
{
	char *pName;

	if (isatty(0)) 
    {
		pName = ttyname(0);
		if (NULL == pName)
			pName = "undefined";
	} 
    else 
    {
		pName = "not a tty";
	}
	printf("fd 0: %s\n", pName);

	if (isatty(1)) 
    {
		pName = ttyname(1);
		if (NULL == pName)
			pName = "undefined";
	} 
    else 
    {
		pName = "not a tty";
	}
	printf("fd 1: %s\n", pName);

	if (isatty(2)) 
    {
		pName = ttyname(2);
        if (NULL == pName)
        {
            pName = "undefined";
        }
	} 
    else 
    {
		pName = "not a tty";
	}

	printf("fd 2: %s\n", pName);

	exit(0);
}
