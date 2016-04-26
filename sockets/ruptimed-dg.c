/*16-8 基于数据报 提供系统的uptime的服务器程序--无连接服务器，客户端16-7*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>

#define BUFLEN		    128
#define MAXADDRLEN	    256

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

extern int initserver(int, struct sockaddr *, socklen_t, int);

/*服务器在recvfrom中阻塞等待服务器请求*/
void serve(int sockfd)
{
	int			n = 0;
	socklen_t	alen;
	FILE		*fp;
	char		buf[BUFLEN];
	char		abuf[MAXADDRLEN];

    memset(buf, 0, sizeof(buf));
    memset(abuf, 0, sizeof(abuf));

	for (;;) 
    {
		alen = MAXADDRLEN;

		if ((n = recvfrom(sockfd, buf, BUFLEN, 0,  \
		  (struct sockaddr *)abuf, &alen)) < 0) 
        {
			syslog(LOG_ERR, "ruptimed: recvfrom error: %s", \
			  strerror(errno));
			exit(1);
		}

		if ((fp = popen("/usr/bin/uptime", "r")) == NULL) 
        {
			sprintf(buf, "error: %s\n", strerror(errno));
            /*recvfrom的地址，原路返回*/
			sendto(sockfd, buf, strlen(buf), 0, \
			  (struct sockaddr *)abuf, alen);
		} 
        else
        {
            if (fgets(buf, BUFLEN, fp) != NULL)
            {
                sendto(sockfd, buf, strlen(buf), 0, \
                    (struct sockaddr *)abuf, alen);
            }

			pclose(fp);
		}
	}
}

int main(int argc, char *argv[])
{
	struct addrinfo	*pAilist, *pAip;
	struct addrinfo	hint;
	int				sockfd = 0, err = 0, n = 0;
	char			*pHost = NULL;

    if (argc != 1)
    {
        err_quit("usage: ruptimed");
    }

#ifdef _SC_HOST_NAME_MAX
	n = sysconf(_SC_HOST_NAME_MAX);
	if (n < 0)	/* best guess */
#endif

    n = HOST_NAME_MAX;
	pHost = malloc(n);
    if (NULL == pHost)
    {
        err_sys("malloc error");
    }

    if (gethostname(pHost, n) < 0)
    {
        err_sys("gethostname error");
    }

	daemonize("ruptimed");

	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	if ((err = getaddrinfo(pHost, "4000", &hint, &pAilist)) != 0) 
    {
		syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s", \
		  gai_strerror(err));
		exit(1);
	}

	for (pAip = pAilist; pAip != NULL; pAip = pAip->ai_next) 
    {
		if ((sockfd = initserver(SOCK_DGRAM, pAip->ai_addr, \
		  pAip->ai_addrlen, 0)) >= 0) 
        {
			serve(sockfd);
			exit(0);
		}
	}
	exit(1);
}
