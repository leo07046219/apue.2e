/*17-22 ����������main�����汾1*/
#include	"opend.h"

char	errmsg[MAXLINE];
int		oflag;
char	*pathname;

int main(void)
{
	int		nread = 0;
	char    buf[MAXLINE];

    memset((char *)buf, 0, sizeof(buf));

	for ( ; ; ) 
    {   
        /* read arg buffer from client, process request */
        if ((nread = read(STDIN_FILENO, buf, MAXLINE)) < 0)
        {
            err_sys("read error on stream pipe");
        }           
        else if (0 == nread)
        {
            break;      /* client has closed the stream pipe */
        }

        request(buf, nread, STDOUT_FILENO);
	}

	exit(0);
}
/* 
��ڲ��������⣺

usage: <pathname> <oflag>
open /mnt/hgfs/win10Folder/apue.2e/open.fe/bufName 777
usage: <pathname> <oflag>

usage: <pathname> <oflag>

*/