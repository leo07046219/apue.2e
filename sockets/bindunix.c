/*17-7 ��һ����ַ��һUNIX���׽���*/

#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>

int main(void)
{
	int fd = 0, size = 0;
	struct sockaddr_un un;

    memset((char *)&un, 0, sizeof(un));

	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, "foo.socket");

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        err_sys("socket failed");
    }

    /*ȷ��sun_path��Ա��sockaddr_un��ƫ��������һ��ȷ���󶨵�ַ���ȣ�
    ��sockaddr_un�ṹ��ͬƽ̨�в���*/
	size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    if (bind(fd, (struct sockaddr *)&un, size) < 0)
    {
        err_sys("bind failed");
    }

	printf("UNIX domain socket bound\n");

	exit(0);
}
