/*14-8 ²âÊÔisastreamº¯Êı*/

#include "apue.h"
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int		i = 0, fd = 0;

	for (i = 1; i < argc; i++) 
    {
		if ((fd = open(argv[i], O_RDONLY)) < 0)
        {
			err_ret("%s: can't open", argv[i]);
			continue;
		}

        if (isastream(fd) == 0)
        {
            err_ret("%s: not a stream\n", argv[i]);
        }
        else
        {
            err_msg("%s: streams device\n", argv[i]);
        }
	}

	exit(0);
}