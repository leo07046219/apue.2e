/*13-2 使用文件和记录锁，保证只运行某个守护进程的副本*/
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

extern int lockfile(int);

int already_running(void)
{
	int		fd;
	char	buf[16];

    memset(buf, 0, sizeof(buf));

	fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	if (fd < 0) 
    {
		syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}

    /*重复的lockfile操作会返回失败--保证了守护进程的单例特性*/
	if (lockfile(fd) < 0) 
    {
		if ((EACCES ==  errno)|| (EAGAIN == errno)) 
        {
			close(fd);
			return(1);
		}
		syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}

    /*文件长度截短的必要性:进程id的字符串可能更短，不截短，之前长id串末尾会保留*/
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);

	return(0);
}
