/*21-4 用于提交打印作业的命令程序*/

/*
 * The client command for printing documents.  Opens the file
 * and sends it to the printer spooling daemon.  Usage:
 * 	print [-t] filename
 */
#include "apue.h"
#include "print.h"
#include <fcntl.h>
#include <pwd.h>

/*
 * Needed for logging funtions.
 如果该整数非零，错误消息将被送到标准错误输出来取代日志文件
 */
int log_to_stderr = 1;

void submit_file(int, int, const char *, size_t, int);

int main(int argc, char *argv[])
{
	int				fd = 0, sockfd = 0, err = 0, text = 0, c = 0;
	struct stat		sbuf;
	char			*host = NULL;
	struct addrinfo	*ailist = NULL, *aip = NULL;

    memset(&sbuf, 0, sizeof(sbuf));

	err = 0;
	text = 0;

    /*getopt()帮助命令开发者按照一直的方式来处理命令行选项*/
	while ((c = getopt(argc, argv, "t")) != -1) 
    {
		switch (c) 
        {
		case 't'://强迫文件按照文本格式打印
			text = 1;
			break;

		case '?':
			err = 1;
			break;

        default:
            break;
		}
	}

    if (err || (optind != argc - 1))
    {
        err_quit("usage: print [-t] filename");
    }
    if ((fd = open(argv[optind], O_RDONLY)) < 0)
    {
        err_sys("print: can't open %s", argv[1]);
    }
    if (fstat(fd, &sbuf) < 0)
    {
        err_sys("print: can't stat %s", argv[1]);
    }

    if (!S_ISREG(sbuf.st_mode))
    {
        err_quit("print: %s must be a regular file\n", argv[1]);
    }

	/*
	 * Get the hostname of the host acting as the print server.
	 */
    if (NULL == (host = get_printserver()))
    {
        err_quit("print: no print server defined");
    }
    /*将主机名字转化为网络地址*/
    if ((err = getaddrlist(host, "print", &ailist)) != 0)
    {
        err_quit("print: getaddrinfo error: %s", gai_strerror(err));
    }
    /*从由getaddrinfo返回的地址列表中，一次使用一个地址来尝试连接到守护进程程序，
    然后使用第一个能够连接的地址来发送文件到守护进程程序*/
	for (aip = ailist; aip != NULL; aip = aip->ai_next) 
    {
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        {
			err = errno;
		} 
        else if (connect_retry(sockfd, aip->ai_addr, aip->ai_addrlen) < 0) 
        {
			err = errno;
		} 
        else 
        {
            /*能够建立一个连接，发送文件到打印假脱机守护进程*/
			submit_file(fd, sockfd, argv[1], sbuf.st_size, text);
			exit(0);
		}
	}

	errno = err;
    /*使用err_ret和exit代替单一调用err_sys以避免编译警告，
    ∵如果单独调用err_sys，main的最后一行就不是return或exit调用了*/
	err_ret("print: can't contact %s", host);
	exit(1);
}

/*
 * Send a file to the printer daemon.
 */
void submit_file(int fd, int sockfd, const char *fname, size_t nbytes, \
            int text)
{
	int					nr = 0, nw = 0, len = 0;
	struct passwd		*pwd = NULL;
	struct printreq		req;
	struct printresp	res;
	char				buf[IOBUFSZ];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    memset(buf, 0, sizeof(buf));

	/*
	 * First build the header -- req.
	 */
    if (NULL == (pwd = getpwuid(geteuid())))//有效用户id-->用户名
    {
        strcpy(req.usernm, "unknown");
    }
    else
    {
        strcpy(req.usernm, pwd->pw_name);
    }

	req.size = htonl(nbytes);

    if (text)
    {
        req.flags = htonl(PR_TEXT);
    }
    else
    {
        req.flags = 0;
    }
    /*将作业名设为要打印的文件名，如果明知长度超过报文所能容纳的长度，
    则将名字的开头部分截断，用省略号取代*/
	if ((len = strlen(fname)) >= JOBNM_MAX) 
    {
		/*
		 * Truncate the filename (+-5 accounts for the leading
		 * four characters and the terminating null).
		 */
		strcpy(req.jobnm, "... ");
		strncat(req.jobnm, &fname[len-JOBNM_MAX+5], JOBNM_MAX-5);
	} 
    else 
    {
		strcpy(req.jobnm, fname);
	}

	/*
	 * Send the header to the server.
	 */
	nw = writen(sockfd, &req, sizeof(struct printreq));
	if (nw != sizeof(struct printreq)) 
    {
        if (nw < 0)
        {
            err_sys("can't write to print server");
        }
        else
        {
            err_quit("short write (%d/%d) to print server", \
                nw, sizeof(struct printreq));
        }

	}

	/*
	 * Now send the file.
     一次读取文件中的IOBUFSZ长度，并发送给守护进程
	 */
	while ((nr = read(fd, buf, IOBUFSZ)) != 0)
    {
		nw = writen(sockfd, buf, nr);
		if (nw != nr)
        {
            if (nw < 0)
            {
                err_sys("can't write to print server");
            }
            else
            {
                err_quit("short write (%d/%d) to print server", \
                    nw, nr);
            }
		}
	}

	/*
	 * Read the response.
	 */
    if ((nr = readn(sockfd, &res, sizeof(struct printresp))) != \
        sizeof(struct printresp))
    {
        err_sys("can't read response from server");
    }

	if (res.retcode != 0)//请求失败
    {
		printf("rejected: %s\n", res.msg);
		exit(1);
	} 
    else 
    {
		printf("job ID %ld\n", ntohl(res.jobid));//守护进程返回的id可用于取消打印作业
	}

	exit(0);
}
/*
一个成功的从守护进程来的相应并不意味着打印机可以打印该文件，
仅仅意味着守护进程成功地将其加入到打印作业队列中
*/