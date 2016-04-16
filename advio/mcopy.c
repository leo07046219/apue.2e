/*14-12 用存储映射I/O复制文件*/

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

    /* need size of input file 获取输入文件长度*/
	if (fstat(fdin, &statbuf) < 0)
    {
        err_sys("fstat error");
    }

	/* set size of output file */
    if (lseek(fdout, statbuf.st_size - 1, SEEK_SET) == -1)
    {
        err_sys("lseek error");
    }
    /*写一个字节以设置输出文件长度，若不这样设置，则对输出文件调用mmap亦可，
    但对相关存储区的第一次引用会产生SIGBUS*/
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

    /*数据被写入文件的确切时间依赖于系统的算法。
    若想确保数据安全地写到文件中，主要在进程终止前以MS_SYNC标志调用msync*/

	exit(0);
}
/*
1.mmap/memcpy方式较read/write方式快，
∵前者做的工作少，用read/write要先将数据从内核缓冲区复制到应用缓冲区(read)，
再将应用缓冲区复制到内核缓冲区(write)
mmap/memcpy时直接将映射到应用程序地址空间的一个内核缓冲区数据复制到另一个应用程序
地址空间的内核缓冲区中
2.mmap/memcpy限制，
某些网络设备、终端设备之间不能使用
*/