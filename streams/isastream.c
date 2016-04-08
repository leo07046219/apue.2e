/*14-7 建仓描述符是否引用STREAMS设备*/

#include	<stropts.h>
#include	<unistd.h>

int isastream(int fd)
{
	return(ioctl(fd, I_CANPUT, 0) != -1);
}