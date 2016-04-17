/*15-4 用popen向分页程序传送文件*/

#include "apue.h"
#include <sys/wait.h>

/* environment variable, or default 
若shell变量PAGER已经定义且其值非空，则使用其值，否则使用字符创more*/
#define	PAGER	"${PAGER:-more}" 

int main(int argc, char *argv[])
{
	char	line[MAXLINE];
	FILE	*fpin, *fpout;

    if (argc != 2)
    {
        err_quit("usage: a.out <pathname>");
    }
    
    memset(line, 0, sizeof(line));

    if ((fpin = fopen(argv[1], "r")) == NULL)
    {
        err_sys("can't open %s", argv[1]);
    }

    if ((fpout = popen(PAGER, "w")) == NULL)
    {
        err_sys("popen error");
    }

	/* copy argv[1] to pager */
	while (fgets(line, MAXLINE, fpin) != NULL) 
    {
        if (fputs(line, fpout) == EOF)
        {
            err_sys("fputs error to pipe");
        }
	}

    if (ferror(fpin))
    {
        err_sys("fgets error");
    }

    if (pclose(fpout) == -1)
    {
        err_sys("pclose error");
    }

	exit(0);
}
/*
1.用popen重写程序清单15-2，更简洁的一种实现；
2.*/