/*14-7 �����������Ƿ�����STREAMS�豸*/

#include	<stropts.h>
#include	<unistd.h>

int isastream(int fd)
{
    /*ʹ��I_CANPUT ioctol�����Ե���������˵�������ȼ������Ƿ��д*/
    return(ioctl(fd, I_CANPUT, 0) != -1);
}