/*21-3 ipp所需工具函数实现*/

#include "apue.h"
#include "print.h"
#include <ctype.h>
#include <sys/select.h>

#define MAXCFGLINE 512  //打印机配置文件中行的最大长度
#define MAXKWLEN   16   //配置文件中关键字的最大尺寸
#define MAXFMTLEN  16   //传给sscanf的格式化字符串最大长度

/*
 * Get the address list for the given host and service and
 * return through ailistpp.  Returns 0 on success or an error
 * code on failure.  Note that we do not set errno if we
 * encounter an error.
 *
 * LOCKING: none.不需要用互斥锁--该注释只为用于多线程锁定的文档编写，
 列出了可能有的关于锁的假设，告知该函数所需要获得或释放的锁，
 并告知调用者这个函数所需要持有的锁。
 */
int getaddrlist(const char *host, const char *service, \
  struct addrinfo **ailistpp)
{
	int				err = 0;
	struct addrinfo	hint;

    memset(&hint, 0, sizeof(hint));

	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	err = getaddrinfo(host, service, &hint, ailistpp);

	return(err);
}

/*
 * Given a keyword, scan the configuration file for a match
 * and return the string value corresponding to the keyword.
 *在打印机配置文件中搜索指定关键字
 * LOCKING: none.
 */
static char *scan_configfile(char *keyword)
{
	int				n = 0, match = 0;
	FILE			*fp = NULL;
	char			keybuf[MAXKWLEN], pattern[MAXFMTLEN];
	char			line[MAXCFGLINE];
	static char		valbuf[MAXCFGLINE];//非线程安全

    memset(keybuf, 0, sizeof(keybuf));
    memset(pattern, 0, sizeof(pattern));
    memset(line, 0, sizeof(line));
    memset(valbuf, 0, sizeof(valbuf));

    if (NULL == (fp = fopen(CONFIG_FILE, "r")))
    {
        log_sys("can't open %s", CONFIG_FILE);
    }
    /*建立一个格式指示器来限定字符串尺寸，
    这样就不会使用于在堆栈中存放字符串的缓冲区溢出
    3个%号？*/
	sprintf(pattern, "%%%ds %%%ds", MAXKWLEN-1, MAXCFGLINE-1);
	match = 0;

	while (fgets(line, MAXCFGLINE, fp) != NULL) 
    {
		n = sscanf(line, pattern, keybuf, valbuf);

		if ((2 == n) && (strcmp(keyword, keybuf) == 0)) 
        {
			match = 1;
			break;
		}
	}

	fclose(fp);
    if (match != 0)
    {
        return(valbuf);
    }
    else
    {
        return(NULL);
    }
}

/*
 * Return the host name running the print server or NULL on error.
 *函数封装器
 * LOCKING: none.
 */
char *get_printserver(void)
{
	return(scan_configfile("printserver"));
}

/*
 * Return the address of the network printer or NULL on error.
 *
 * LOCKING: none.
 */
struct addrinfo *get_printaddr(void)
{
	int				err  = 0;
	char			*p = NULL;
	struct addrinfo	*ailist = NULL;

	if ((p = scan_configfile("printer")) != NULL) 
    {
		if ((err = getaddrlist(p, "ipp", &ailist)) != 0) 
        {
			log_msg("no address information for %s", p);
			return(NULL);
		}
		return(ailist);
	}
	log_msg("no printer address specified");
	return(NULL);
}

/*
 * "Timed" read - timout specifies the # of seconds to wait before
 * giving up (5th argument to select controls how long to wait for
 * data to be readable).  Returns # of bytes read or -1 on error.
 *带超时读--在打印假脱机守护进程上用于预防拒绝服务攻击
 恶意用户可能长恩能上尝试连接到服务器进程而不发送数据，不让其他用户提交打印作业
 巧妙之处在于，选择一个合理的超时值
 input:超时时间--秒
 * LOCKING: none.
 */
ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timout)
{
	int				nfds = 0;
	fd_set			readfds;
	struct timeval	tv;

	tv.tv_sec = timout;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	nfds = select(fd+1, &readfds, NULL, NULL, &tv);
	if (nfds <= 0) 
    {
        if (0 == nfds)
        {
            errno = ETIME;
        }
		return(-1);
	}

	return(read(fd, buf, nbytes));
}

/*
 * "Timed" read - timout specifies the number of seconds to wait
 * per read call before giving up, but read exactly nbytes bytes.
 * Returns number of bytes read or -1 on error.
 *
 * LOCKING: none.
 */
ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timout)
{
	size_t	nleft;
	ssize_t	nread;

	nleft = nbytes;
	while (nleft > 0) 
    {
		if ((nread = tread(fd, buf, nleft, timout)) < 0)
        {
            if (nleft == nbytes)//一个字节都没有读到，tread就返回失败--treadn返回失败
            {
                return(-1); /* error, return -1 */
            }
            else//读到一些字节后tread返回失败--treadn返回读到的数据，没有读完nbytes
            {
                break;      /* error, return amount read so far */
            }
		} 
        else if (0 == nread) //读完所有nbytes数据
        {
			break;          /* EOF */
		}

		nleft -= nread;
		buf += nread;
	}

	return(nbytes - nleft);      /* return >= 0 */
}
