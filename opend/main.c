/*17-29 服务器进程main函数版本2,*/

#include	"opend.h"
#include	<syslog.h>

int     debug, oflag, client_size, log_to_stderr;
char    errmsg[MAXLINE];
char    *pathname;
Client  *client = NULL;

int main(int argc, char *argv[])
{
	int c = 0;

	log_open("open.serv", LOG_PID, LOG_USER);

	opterr = 0;         /* don't want getopt() writing to stderr */
	while ((c = getopt(argc, argv, "d")) != EOF) 
    {
		switch (c) 
        {
		case 'd':       /* debug -d选项调用服务器进程，以交互方式运行*/
			debug = log_to_stderr = 1;
			break;

		case '?':
			err_quit("unrecognized option: -%c", optopt);
		}
	}

    if (0 == debug)
    {
        daemonize("opend");
    }

    /*17-30.loop.select.c, 17-31.loop.poll.c*/
	loop();             /* never returns */
}
