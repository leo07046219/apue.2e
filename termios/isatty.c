/*18-4 POSIX.1 isatty����ʵ��*/

#include	<termios.h>

int isatty(int fd)
{
	struct termios	ts;

	return(tcgetattr(fd, &ts) != -1); /* true if no error (is a tty) */
}
