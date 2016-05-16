/*19-1 基于STREAM的伪终端打开函数*/

#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <stropts.h>

/*
使用open打开克隆设备/dev/ptmx，得到PTY主设备的文件描述符，自动锁定从设备
*/
int ptym_open(char *pts_name, int pts_namesz)
{
	char	*ptr = NULL;
	int		fdm = 0;

	/*
	 * Return the name of the master device so that on failure
	 * the caller can print an error message.  Null terminate
	 * to handle case where strlen("/dev/ptmx") > pts_namesz.
	 */
	strncpy(pts_name, "/dev/ptmx", pts_namesz);
	pts_name[pts_namesz - 1] = '\0';

    if ((fdm = open(pts_name, O_RDWR)) < 0)
    {
        return(-1);
    }

    /* grant access to slave 改变从设备访问权限*/
	if (grantpt(fdm) < 0) 
    {		
		close(fdm);
		return(-2);
	}

    /* clear slave's lock flag 打开从设备之前，必须清除设备内部锁*/
	if (unlockpt(fdm) < 0) 
    {	
		close(fdm);
		return(-3);
	}

    /* get slave's name */
	if (NULL == (ptr = ptsname(fdm))) 
    {	
		close(fdm);
		return(-4);
	}

	/*
	 * Return name of slave.  Null terminate to handle
	 * case where strlen(ptr) > pts_namesz.
	 */
	strncpy(pts_name, ptr, pts_namesz);
	pts_name[pts_namesz - 1] = '\0';
    
    /* return fd of master */
	return(fdm);			
}

/*真正打开一个从设备*/
int ptys_open(char *pts_name)
{
	int		fds = 0, setup = 0;

	/*
	 * The following open should allocate a controlling terminal.
	 */
    if ((fds = open(pts_name, O_RDWR)) < 0)
    {
        return(-5);
    }

	/*
	 * Check if stream is already set up by autopush facility.  图19-2
	 */
	if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0) 
    {
		close(fds);
		return(-6);
	}

    /*如果流中还没压入以下三模块，则压入*/
	if (0 == setup) 
    {
        /*终端仿真模块*/
		if (ioctl(fds, I_PUSH, "ptem") < 0) 
        {
			close(fds);
			return(-7);
		}
        /*终端行规程模块*/
		if (ioctl(fds, I_PUSH, "ldterm") < 0) 
        {
			close(fds);
			return(-8);
		}
        /*提供向早期系统的ioctl兼容性*/
		if (ioctl(fds, I_PUSH, "ttcompat") < 0) 
        {
			close(fds);
			return(-9);
		}
	}

	return(fds);
}
