/*16-4 用户获取服务器uptime的客户端命令*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>

#define MAXADDRLEN	256
#define BUFLEN		128

extern int connect_retry(int, const struct sockaddr *, socklen_t);

/*读取并显示服务器发送的数据*/
void print_uptime(int sockfd)
{
	int		n = 0;
	char	buf[BUFLEN];

    memset(buf, 0, sizeof(buf));

    /*面向连接的，循环读取直到返回0*/
    while ((n = recv(sockfd, buf, BUFLEN, 0)) > 0)
    {
        write(STDOUT_FILENO, buf, n);
    }

    if (n < 0)
    {
        err_sys("recv error");
    }
}

int main(int argc, char *argv[])
{
	struct  addrinfo	*pAilist = NULL, *pAip = NULL;
	struct  addrinfo	hint;
	int     sockfd = 0, err = 0;

    if (argc != 2)
    {
        err_quit("usage: ruptime hostname");
    }

    memset((char *)&hint, 0, sizeof(hint));

	hint.ai_flags = AI_PASSIVE;
	hint.ai_family = 0;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr = NULL;
	hint.ai_next = NULL;

    /*如果服务器支持多重网络接口或多重网络协议，getaddrinfo()会返回不止一个候选地址*/
    if ((err = getaddrinfo(argv[1], "4000", &hint, &pAilist)) != 0)
    {
        err_quit("getaddrinfo error: %s", gai_strerror(err));
    }
    /*轮流尝试每个地址，直到找到一个允许连接服务的地址*/
	for (pAip = pAilist; pAip != NULL; pAip = pAip->ai_next) 
    {
        if ((sockfd = socket(pAip->ai_family, SOCK_STREAM, 0)) < 0)
        {
            err = errno;
        }
        /*面向连接的协议，需要在交换数据前连接服务器*/
		if (connect_retry(sockfd, pAip->ai_addr, pAip->ai_addrlen) < 0) 
        {
			err = errno;
		} 
        else 
        {
			print_uptime(sockfd);
			exit(0);
		}
	}

	fprintf(stderr, "can't connect to %s: %s\n", argv[1], strerror(err));

	exit(1);
}
/*
1.与服务器通信以获得系统命令uptime的输出；
2.入口参数是hostname  ？
以端口号的方式连接服务器：这个就有一点需要注意，hint的ai_flags不要设为AI_PASSIVE，因为返回的地址不是用来监听绑定的，一般设为AI_CANONNAME
apue.2e/sockets$ ./ruptime ubuntu
08:51:29 up 4 days,  2:21,  4 users,  load average: 0.00, 0.01, 0.05

*/