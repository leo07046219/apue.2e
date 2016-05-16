/*19-1 ����STREAM��α�ն˴򿪺���*/

#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <stropts.h>

/*
ʹ��open�򿪿�¡�豸/dev/ptmx���õ�PTY���豸���ļ����������Զ��������豸
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

    /* grant access to slave �ı���豸����Ȩ��*/
	if (grantpt(fdm) < 0) 
    {		
		close(fdm);
		return(-2);
	}

    /* clear slave's lock flag �򿪴��豸֮ǰ����������豸�ڲ���*/
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

/*������һ�����豸*/
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
	 * Check if stream is already set up by autopush facility.  ͼ19-2
	 */
	if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0) 
    {
		close(fds);
		return(-6);
	}

    /*������л�ûѹ��������ģ�飬��ѹ��*/
	if (0 == setup) 
    {
        /*�ն˷���ģ��*/
		if (ioctl(fds, I_PUSH, "ptem") < 0) 
        {
			close(fds);
			return(-7);
		}
        /*�ն��й��ģ��*/
		if (ioctl(fds, I_PUSH, "ldterm") < 0) 
        {
			close(fds);
			return(-8);
		}
        /*�ṩ������ϵͳ��ioctl������*/
		if (ioctl(fds, I_PUSH, "ttcompat") < 0) 
        {
			close(fds);
			return(-9);
		}
	}

	return(fds);
}
