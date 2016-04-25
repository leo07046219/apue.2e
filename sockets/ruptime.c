/*16-4 �û���ȡ������uptime�Ŀͻ�������*/

#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>

#define MAXADDRLEN	256
#define BUFLEN		128

extern int connect_retry(int, const struct sockaddr *, socklen_t);

/*��ȡ����ʾ���������͵�����*/
void print_uptime(int sockfd)
{
	int		n = 0;
	char	buf[BUFLEN];

    memset(buf, 0, sizeof(buf));

    /*�������ӵģ�ѭ����ȡֱ������0*/
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

    /*���������֧�ֶ�������ӿڻ��������Э�飬getaddrinfo()�᷵�ز�ֹһ����ѡ��ַ*/
    if ((err = getaddrinfo(argv[1], "4000", &hint, &pAilist)) != 0)
    {
        err_quit("getaddrinfo error: %s", gai_strerror(err));
    }
    /*��������ÿ����ַ��ֱ���ҵ�һ���������ӷ���ĵ�ַ*/
	for (pAip = pAilist; pAip != NULL; pAip = pAip->ai_next) 
    {
        if ((sockfd = socket(pAip->ai_family, SOCK_STREAM, 0)) < 0)
        {
            err = errno;
        }
        /*�������ӵ�Э�飬��Ҫ�ڽ�������ǰ���ӷ�����*/
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
1.�������ͨ���Ի��ϵͳ����uptime�������
2.��ڲ�����hostname  ��
�Զ˿ںŵķ�ʽ���ӷ��������������һ����Ҫע�⣬hint��ai_flags��Ҫ��ΪAI_PASSIVE����Ϊ���صĵ�ַ�������������󶨵ģ�һ����ΪAI_CANONNAME
apue.2e/sockets$ ./ruptime ubuntu
08:51:29 up 4 days,  2:21,  4 users,  load average: 0.00, 0.01, 0.05

*/