/*17-7 将一个地址绑定一UNIX域套接字*/

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

    /*确定sun_path成员在sockaddr_un的偏移量，进一步确定绑定地址长度，
    ∵sockaddr_un结构不同平台有差异*/
	size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    if (bind(fd, (struct sockaddr *)&un, size) < 0)
    {
        err_sys("bind failed");
    }

	printf("UNIX domain socket bound\n");

	exit(0);
}
