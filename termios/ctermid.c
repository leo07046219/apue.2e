/*18-3 POSIX.1 ctermid������ʵ��*/

#include	<stdio.h>
#include	<string.h>

static char	ctermid_name[L_ctermid];

char * ctermid(char *str)
{
    if (NULL == str)
    {
        str = ctermid_name;
    }

	return(strcpy(str, "/dev/tty"));	/* strcpy() returns str */
}
