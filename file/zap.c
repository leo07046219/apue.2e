/*4-6 utime����ʵ��*/
#include "apue.h"
#include <fcntl.h>
#include <utime.h>

int main(int argc, char *argv[])
{
	int				i = 0, fd = 0;
	struct stat		statbuf;
	struct utimbuf	timebuf;

    memset(&statbuf, 0, sizeof(statbuf));
    memset(&timebuf, 0, sizeof(timebuf));

	for (i = 1; i < argc; i++) 
    {
        /*�����ļ�����ʱ����޸�ʱ��*/
		if (stat(argv[i], &statbuf) < 0) 
        {	
            /* fetch current times */
			err_ret("%s: stat error", argv[i]);
			continue;
		}

        /*�ض��ļ�*/
		if ((fd = open(argv[i], O_RDWR | O_TRUNC)) < 0) 
        { 
            /* truncate */
			err_ret("%s: open error", argv[i]);
			continue;
		}
		close(fd);

        /*�ָ�ʱ��*/
		timebuf.actime  = statbuf.st_atime;
		timebuf.modtime = statbuf.st_mtime;

		if (utime(argv[i], &timebuf) < 0) 
        {		
            /* reset times */
			err_ret("%s: utime error", argv[i]);
			continue;
		}
	}

	exit(0);
}
