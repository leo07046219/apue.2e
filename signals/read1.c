/*10-7 ���г�ʱ���Ƶ�read����*/

#include "apue.h"

static void	sig_alrm(int);

int main(void)
{
	int		n = 0;
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

    if (signal(SIGALRM, sig_alrm) == SIG_ERR)
    {
        err_sys("signal(SIGALRM) error");
    }

	alarm(10);

    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0)
    {
        err_sys("read error");
    }

	alarm(0);

	write(STDOUT_FILENO, line, n);
	exit(0);
}

static void sig_alrm(int signo)
{
	/* nothing to do, just return to interrupt the read */
}

/*
�Կ��������Ĳ�������ʱ������ֵ����ʱֹͣ����

���⣺
1.alarm���ú�read����֮���о����������ں���alarm���ú�read����֮��ʹ����������
����ʱ�䳬������ʱ�䣬read������Զ����--һ�㳬ʱʱ�����ýϳ���1���ӣ��þ����������ʽϵ�
2.��ϵͳ�������Զ������ģ��򵱴�SIGALRM�źŴ�����򷵻�ʱ��read�������жϡ�
�����������ʱ�����Ʋ������á�
*/
