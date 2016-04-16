/*14-12 �ô洢ӳ��I/O�����ļ�*/

#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int			fdin = 0, fdout = 0;
	void		*pSrc, *pDst;
	struct stat	statbuf;

    if (argc != 3)
    {
        err_quit("usage: %s <fromfile> <tofile>", argv[0]);
    }

    if ((fdin = open(argv[1], O_RDONLY)) < 0)
    {
        err_sys("can't open %s for reading", argv[1]);
    }

    if ((fdout = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0)
    {
        err_sys("can't creat %s for writing", argv[2]);
    }

    /* need size of input file ��ȡ�����ļ�����*/
	if (fstat(fdin, &statbuf) < 0)
    {
        err_sys("fstat error");
    }

	/* set size of output file */
    if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1)
    {
        err_sys("lseek error");
    }
    /*дһ���ֽ�����������ļ����ȣ������������ã��������ļ�����mmap��ɣ�
    ������ش洢���ĵ�һ�����û����SIGBUS*/
    if (write(fdout, "", 1) != 1)
    {
        err_sys("write error");
    }

    if ((pSrc = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, \
        fdin, 0)) == MAP_FAILED)
    {
        err_sys("mmap error for input");
    }

    if ((pDst = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE, \
        MAP_SHARED, fdout, 0)) == MAP_FAILED)
    {
        err_sys("mmap error for output");
    }

	memcpy(pDst, pSrc, statbuf.st_size);	/* does the file copy */

    /*���ݱ�д���ļ���ȷ��ʱ��������ϵͳ���㷨��
    ����ȷ�����ݰ�ȫ��д���ļ��У���Ҫ�ڽ�����ֹǰ��MS_SYNC��־����msync*/

	exit(0);
}
/*
1.mmap/memcpy��ʽ��read/write��ʽ�죬
��ǰ�����Ĺ����٣���read/writeҪ�Ƚ����ݴ��ں˻��������Ƶ�Ӧ�û�����(read)��
�ٽ�Ӧ�û��������Ƶ��ں˻�����(write)
mmap/memcpyʱֱ�ӽ�ӳ�䵽Ӧ�ó����ַ�ռ��һ���ں˻��������ݸ��Ƶ���һ��Ӧ�ó���
��ַ�ռ���ں˻�������
2.mmap/memcpy���ƣ�
ĳЩ�����豸���ն��豸֮�䲻��ʹ��
*/