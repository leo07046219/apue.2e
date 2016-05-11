/*18-6 POSIX.1 ttyname函数实现 */

#include	<sys/stat.h>
#include	<dirent.h>
#include	<limits.h>
#include	<string.h>
#include	<termios.h>
#include	<unistd.h>
#include	<stdlib.h>

struct devdir 
{
	struct devdir	*d_next;
	char			*d_name;
};

/*head,tail是用来干嘛的?*/
static struct devdir	*head;
static struct devdir	*tail;
static char				pathname[_POSIX_PATH_MAX + 1];

static void add(char *dirname)
{
	struct devdir	*ddp = NULL;
	int				len = 0;

	len = strlen(dirname);

	/*
	 * Skip ., .., and /dev/fd.
	 */
    if ((dirname[len - 1] == '.') && (dirname[len - 2] == '/' ||
        (dirname[len - 2] == '.' && dirname[len - 3] == '/')))
    {
        return;
    }

    if (strcmp(dirname, "/dev/fd") == 0)
    {
        return;
    }
	ddp = malloc(sizeof(struct devdir));

    if (NULL == ddp)
    {
        return;
    }
    /*带malloc的字符串拷贝函数，和free()成对使用*/
	ddp->d_name = strdup(dirname);
	if (NULL == ddp->d_name) 
    {
		free(ddp);
		return;
	}
	ddp->d_next = NULL;
	if (NULL == tail) 
    {
		head = ddp;
		tail = ddp;
	} 
    else 
    {
		tail->d_next = ddp;
		tail = ddp;
	}
}

static void cleanup(void)
{
	struct devdir	*ddp, *nddp;

	ddp = head;
	while (ddp != NULL) 
    {
		nddp = ddp->d_next;
		free(ddp->d_name);
		free(ddp);
		ddp = nddp;
	}

	head = NULL;
	tail = NULL;
}

static char *searchdir(char *dirname, struct stat *fdstatp)
{
	struct stat		devstat;
	DIR				*dp = NULL;
	int				devlen = 0;
	struct dirent	*dirp = NULL;

    memset(&devstat, 0, sizeof(devstat));

	strcpy(pathname, dirname);
    if (NULL == (dp = opendir(dirname)))
    {
        return(NULL);
    }

	strcat(pathname, "/");
	devlen = strlen(pathname);

	while ((dirp = readdir(dp)) != NULL) 
    {
		strncpy(pathname + devlen, dirp->d_name, \
		  _POSIX_PATH_MAX - devlen);

		/*
		 * Skip aliases.
		 */
        if (strcmp(pathname, "/dev/stdin") == 0 || \
            strcmp(pathname, "/dev/stdout") == 0 || \
            strcmp(pathname, "/dev/stderr") == 0)
        {
            continue;
        }

        if (stat(pathname, &devstat) < 0)
        {
            continue;
        }

		if (S_ISDIR(devstat.st_mode)) 
        {
			add(pathname);
			continue;
		}

		if (devstat.st_ino == fdstatp->st_ino && \
		  devstat.st_dev == fdstatp->st_dev) 
        {	
            /* found a match */
			closedir(dp);
			return(pathname);
		}
	}

	closedir(dp);
	return(NULL);
}

char *ttyname(int fd)
{
	struct stat		fdstat;
	struct devdir	*ddp;
	char			*rval;

    if (isatty(fd) == 0)
    {
        return(NULL);
    }
    if (fstat(fd, &fdstat) < 0)
    {
        return(NULL);
    }
    if (S_ISCHR(fdstat.st_mode) == 0)
    {
        return(NULL);
    }

	rval = searchdir("/dev", &fdstat);
	if (NULL == rval) 
    {
        for (ddp = head; ddp != NULL; ddp = ddp->d_next)
        {
            if ((rval = searchdir(ddp->d_name, &fdstat)) != NULL)
            {
                break;
            }
        }
	}

	cleanup();
	return(rval);
}
/*
读/dev目录(/dev下整个文件系统子树)，寻找具有相同设备号和i节点编号的表项。
为什么设备号和i节点编号相等，就说明找到了？
*/