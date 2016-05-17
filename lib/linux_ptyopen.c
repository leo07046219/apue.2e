/*19-3 Linux的伪终端open函数*/

#include "apue.h"
#include <fcntl.h>

#ifndef _HAS_OPENPT
int posix_openpt(int oflag)
{
	int		fdm = 0;

	fdm = open("/dev/ptmx", oflag);
	return(fdm);
}
#endif

#ifndef _HAS_PTSNAME
char *ptsname(int fdm)
{
	int			sminor = 0;
	static char	pts_name[16];

    if (ioctl(fdm, TIOCGPTN, &sminor) < 0)
    {
        return(NULL);
    }
	snprintf(pts_name, sizeof(pts_name), "/dev/pts/%d", sminor);
	return(pts_name);
}
#endif

#ifndef _HAS_GRANTPT
/*Linux中，PTY从设备已为组tty所拥有，所以在grantpt中须做的一切是确保访问缺陷是正确的*/
int grantpt(int fdm)
{
	char *pts_name = NULL;

	pts_name = ptsname(fdm);
	return(chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP));
}
#endif

#ifndef _HAS_UNLOCKPT
int unlockpt(int fdm)
{
	int lock = 0;

	return(ioctl(fdm, TIOCSPTLCK, &lock));
}
#endif

int ptym_open(char *pts_name, int pts_namesz)
{
	char	*ptr = NULL;
	int		fdm = 0;

	/*
	 * Return the name of the master device so that on failure
	 * the caller can print an error message.  Null terminate
	 * to handle case where string length > pts_namesz.
	 */
	strncpy(pts_name, "/dev/ptmx", pts_namesz);
	pts_name[pts_namesz - 1] = '\0';

	fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
    {
        return(-1);
    }

	if (grantpt(fdm) < 0) 
    {		
        /* grant access to slave */
		close(fdm);
		return(-2);
	}
	if (unlockpt(fdm) < 0) 
    {	
        /* clear slave's lock flag */
		close(fdm);
		return(-3);
	}
	if (NULL == (ptr = ptsname(fdm))) 
    {	
        /* get slave's name */
		close(fdm);
		return(-4);
	}

	/*
	 * Return name of slave.  Null terminate to handle case
	 * where strlen(ptr) > pts_namesz.
	 */
	strncpy(pts_name, ptr, pts_namesz);
	pts_name[pts_namesz - 1] = '\0';

    /* return fd of master */
	return(fdm);			
}

int ptys_open(char *pts_name)
{
	int fds = 0;

    if ((fds = open(pts_name, O_RDWR)) < 0)
    {
        return(-5);
    }

	return(fds);
}
