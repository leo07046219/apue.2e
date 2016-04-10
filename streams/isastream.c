/*14-7 建仓描述符是否引用STREAMS设备*/

#include	<stropts.h>
#include	<unistd.h>

int isastream(int fd)
{
    /*使用I_CANPUT ioctol来测试第三个参数说明的优先级波段是否可写*/
    return(ioctl(fd, I_CANPUT, 0) != -1);
}