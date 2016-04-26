/*16-7 采用数据报服务的客户端命令--无连接客户端，服务器16-8*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>

#define BUFLEN		128
#define TIMEOUT		20

void sigalrm(int signo)
{
}

void print_uptime(int sockfd, struct addrinfo *pAip)
{
	int		n;
	char	buf[BUFLEN];

    memset(buf, 0, sizeof(buf));

	buf[0] = 0;
    if (sendto(sockfd, buf, 1, 0, pAip->ai_addr, pAip->ai_addrlen) < 0)
    {
        err_sys("sendto error");
    }

	alarm(TIMEOUT);
	if ((n = recvfrom(sockfd, buf, BUFLEN, 0, NULL, NULL)) < 0) 
    {
        if (errno != EINTR)
        {
            alarm(0);
        }
		err_sys("recv error");
	}

	alarm(0);

	write(STDOUT_FILENO, buf, n);
}

int main(int argc, char *argv[])
{
	struct addrinfo		*pAilist =NULL, *pAip = NULL;
	struct addrinfo		hint;
	int					sockfd = 0, err = 0;
	struct sigaction	sa;

    if (argc != 2)
    {
        err_quit("usage: ruptime hostname");
    }

    memset((char *)&hint, 0, sizeof(hint));
    memset((char *)&sa, 0, sizeof(sa));

	sa.sa_handler = sigalrm;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

    /*如果服务器不在运行状态，客户端调用recvfrom就会无限期阻塞，
    需要在recvfrom之前设置警告时钟*/
    if (sigaction(SIGALRM, &sa, NULL) < 0)
    {
        err_sys("sigaction error");
    }

	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

    if ((err = getaddrinfo(argv[1], "4000", &hint, &pAilist)) != 0)
    {
        err_quit("getaddrinfo error: %s", gai_strerror(err));
    }

	for (pAip = pAilist; pAip != NULL; pAip = pAip->ai_next) 
    {
		if ((sockfd = socket(pAip->ai_family, SOCK_DGRAM, 0)) < 0) 
        {
			err = errno;
		} 
        else 
        {
			print_uptime(sockfd, pAip);
			exit(0);
		}
	}

	fprintf(stderr, "can't contact %s: %s\n", argv[1], strerror(err));

	exit(1);
}
