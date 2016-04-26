/*16-6 ������ʾ����ֱ��д���׽��ֵķ���������--��һ���������ӵķ�����*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define QLEN            10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

extern int initserver(int, struct sockaddr *, socklen_t, int);

/*�ڸ���accept���տͻ���������ػ������У����ӽ��̣�
ʹ��dup2ʹ�ӽ��̵�STDIN_FILENO�ĸ����򿪵�/dev/null,
STDOUT_FILENO STDERR_FILENO�򿪵��׽��ֶ˵㣬
����uptime������*/
void serve(int sockfd)
{
	int		clfd = 0, status = 0;
	pid_t	pid;

	for (;;) 
    {
		clfd = accept(sockfd, NULL, NULL);

		if (clfd < 0) 
        {
			syslog(LOG_ERR, "ruptimed: accept error: %s",
			  strerror(errno));
			exit(1);
		}

		if ((pid = fork()) < 0) 
        {
			syslog(LOG_ERR, "ruptimed: fork error: %s",
			  strerror(errno));
			exit(1);
		} 
        else if (0 == pid) 
        {	
            /* child */

			/*
			 * The parent called daemonize ({Prog daemoninit}), so
			 * STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO
			 * are already open to /dev/null.  Thus, the call to
			 * close doesn't need to be protected by checks that
			 * clfd isn't already equal to one of these values.
			 */
			if (dup2(clfd, STDOUT_FILENO) != STDOUT_FILENO || \
			  dup2(clfd, STDERR_FILENO) != STDERR_FILENO) 
            {
				syslog(LOG_ERR, "ruptimed: unexpected error");
				exit(1);
			}
			close(clfd);

			execl("/usr/bin/uptime", "uptime", (char *)0);
			syslog(LOG_ERR, "ruptimed: unexpected return from exec: %s", \
			  strerror(errno));
		} 
        else 
        {		
            /* parent */
			close(clfd);
			waitpid(pid, &status, 0);
		}
	}
}

int main(int argc, char *argv[])
{
	struct addrinfo	*ailist, *aip;
	struct addrinfo	hint;
	int				sockfd, err, n;
	char			*host;

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

	daemonize("ruptimed");

	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

	if ((err = getaddrinfo(host, "4000", &hint, &ailist)) != 0) 
    {
		syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s", \
		  gai_strerror(err));
		exit(1);
	}

	for (aip = ailist; aip != NULL; aip = aip->ai_next) 
    {
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
1.����������uptime����ı�׼����ͱ�׼�����滻Ϊ���ӵ��ͻ��˵��׽��ֶ˵㣻
2.�����̿��԰�ȫ�عر����ӵ��ͻ��˵��ļ�����������uptimeʱ�䲻���������̿��Եȴ���
3.������ VS ��������  ȡ���ڹ�����ʵʱ�Լ��Դ�������̶̳ȣ�
�������ӣ���ζ����Ҫ�����ʱ�����Դ���ݴ�
*/