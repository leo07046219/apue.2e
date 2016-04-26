/*16-5 提供系统uptime的服务器程序---面向连接的服务器,配合16-4*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>

#define BUFLEN          128
#define QLEN            10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

extern int initserver(int, struct sockaddr *, socklen_t, int);

/*接收连接请求，一旦有新的连接建立，就把uptime发给该连接的对端*/
void serve(int sockfd)
{
	int		clfd = 0;
	FILE	*fp;
	char	buf[BUFLEN];

    memset(buf, 0, sizeof(buf));

	for (;;) 
    {
		clfd = accept(sockfd, NULL, NULL);

		if (clfd < 0) 
        {
			syslog(LOG_ERR, "ruptimed: accept error: %s",
			  strerror(errno));

			exit(1);
		}

		if ((fp = popen("/usr/bin/uptime", "r")) == NULL) 
        {
			sprintf(buf, "error: %s\n", strerror(errno));
			send(clfd, buf, strlen(buf), 0);
		} 
        else 
        {
            while (fgets(buf, BUFLEN, fp) != NULL)
            {
                send(clfd, buf, strlen(buf), 0);
            }

			pclose(fp);
		}

		close(clfd);
	}
}

int main(int argc, char *argv[])
{
	struct addrinfo *ailist = NULL, *aip = NULL;
	struct addrinfo hint;
	int     sockfd = 0, err = 0, n = 0;
	char    *host;

    if (argc != 1)
    {
        err_quit("usage: ruptimed");
    }

#ifdef _SC_HOST_NAME_MAX
	n = sysconf(_SC_HOST_NAME_MAX);
	if (n < 0)	/* best guess */
#endif

    n = HOST_NAME_MAX;
	host = malloc(n);

    if (NULL == host)
    {
        err_sys("malloc error");
    }
    if (gethostname(host, n) < 0)
    {
        err_sys("gethostname error");
    }

    /*守护进程化*/
	daemonize("ruptimed");

	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;
    /*根据获取到的主机名字，查看远程uptime服务地址*/
	if ((err = getaddrinfo(host, "4000", &hint, &ailist)) != 0) 
    {
		syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s", \
		  gai_strerror(err));

		exit(1);
	}

	for (aip = ailist; aip != NULL; aip = aip->ai_next) 
    {
        /*用第一个地址初始化服务器套接字端点*/
		if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, \
		  aip->ai_addrlen, QLEN)) >= 0) 
        {
			serve(sockfd);
			exit(0);
		}
	}

	exit(1);
}
/*
1.守护进程没有起来， ps -e 没有   没事

服务器程序用端口号的形式发布：这种情况下就没有必要在/etc/services里登记了，但是hint结构的ai_flags的参数设定有注意的地方，
在上面的那种情况，因为主机名和服务名都明确提供了，所以即使ai_flags设为0也能返回正确的结果，但是现在我们将第二个参数设为端口号，
这样的话，我们必须将hint结构的ai_flags设为AI_PASSIVE，这样返回的结果才能用于后面的监听绑定，
否则不能，因为AI_PASSIVE就是告诉getaddrinfo返回的地址是用于监听绑定的。

apue.2e/sockets$ ./ruptimed
*/