/*3-1 �����ܷ�Ա�׼��������ƫ����*/
#include "apue.h"

int main(void)
{
    /*����ļ����������õ���һ���ܵ���FIFO���������׽��֣�����-1��errno����ΪESPIPE*/
    if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1)
    {
		printf("cannot seek\n");
    }
    else
    {
		printf("seek OK\n");
    }

	exit(0);
}
