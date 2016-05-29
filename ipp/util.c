/*21-3 ipp���蹤�ߺ���ʵ��*/

#include "apue.h"
#include "print.h"
#include <ctype.h>
#include <sys/select.h>

#define MAXCFGLINE 512  //��ӡ�������ļ����е���󳤶�
#define MAXKWLEN   16   //�����ļ��йؼ��ֵ����ߴ�
#define MAXFMTLEN  16   //����sscanf�ĸ�ʽ���ַ�����󳤶�

/*
 * Get the address list for the given host and service and
 * return through ailistpp.  Returns 0 on success or an error
 * code on failure.  Note that we do not set errno if we
 * encounter an error.
 *
 * LOCKING: none.����Ҫ�û�����--��ע��ֻΪ���ڶ��߳��������ĵ���д��
 �г��˿����еĹ������ļ��裬��֪�ú�������Ҫ��û��ͷŵ�����
 ����֪�����������������Ҫ���е�����
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
 *�ڴ�ӡ�������ļ�������ָ���ؼ���
 * LOCKING: none.
 */
static char *scan_configfile(char *keyword)
{
	int				n = 0, match = 0;
	FILE			*fp = NULL;
	char			keybuf[MAXKWLEN], pattern[MAXFMTLEN];
	char			line[MAXCFGLINE];
	static char		valbuf[MAXCFGLINE];//���̰߳�ȫ

    memset(keybuf, 0, sizeof(keybuf));
    memset(pattern, 0, sizeof(pattern));
    memset(line, 0, sizeof(line));
    memset(valbuf, 0, sizeof(valbuf));

    if (NULL == (fp = fopen(CONFIG_FILE, "r")))
    {
        log_sys("can't open %s", CONFIG_FILE);
    }
    /*����һ����ʽָʾ�����޶��ַ����ߴ磬
    �����Ͳ���ʹ�����ڶ�ջ�д���ַ����Ļ��������
    3��%�ţ�*/
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
 *������װ��
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
 *����ʱ��--�ڴ�ӡ���ѻ��ػ�����������Ԥ���ܾ����񹥻�
 �����û����ܳ������ϳ������ӵ����������̶����������ݣ����������û��ύ��ӡ��ҵ
 ����֮�����ڣ�ѡ��һ������ĳ�ʱֵ
 input:��ʱʱ��--��
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
            if (nleft == nbytes)//һ���ֽڶ�û�ж�����tread�ͷ���ʧ��--treadn����ʧ��
            {
                return(-1); /* error, return -1 */
            }
            else//����һЩ�ֽں�tread����ʧ��--treadn���ض��������ݣ�û�ж���nbytes
            {
                break;      /* error, return amount read so far */
            }
		} 
        else if (0 == nread) //��������nbytes����
        {
			break;          /* EOF */
		}

		nleft -= nread;
		buf += nread;
	}

	return(nbytes - nleft);      /* return >= 0 */
}
