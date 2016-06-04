/*21-4 �����ύ��ӡ��ҵ���������*/

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
 ������������㣬������Ϣ�����͵���׼���������ȡ����־�ļ�
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

    /*getopt()����������߰���һֱ�ķ�ʽ������������ѡ��*/
	while ((c = getopt(argc, argv, "t")) != -1) 
    {
		switch (c) 
        {
		case 't'://ǿ���ļ������ı���ʽ��ӡ
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
    /*����������ת��Ϊ�����ַ*/
    if ((err = getaddrlist(host, "print", &ailist)) != 0)
    {
        err_quit("print: getaddrinfo error: %s", gai_strerror(err));
    }
    /*����getaddrinfo���صĵ�ַ�б��У�һ��ʹ��һ����ַ���������ӵ��ػ����̳���
    Ȼ��ʹ�õ�һ���ܹ����ӵĵ�ַ�������ļ����ػ����̳���*/
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
            /*�ܹ�����һ�����ӣ������ļ�����ӡ���ѻ��ػ�����*/
			submit_file(fd, sockfd, argv[1], sbuf.st_size, text);
			exit(0);
		}
	}

	errno = err;
    /*ʹ��err_ret��exit���浥һ����err_sys�Ա�����뾯�棬
    �������������err_sys��main�����һ�оͲ���return��exit������*/
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
    if (NULL == (pwd = getpwuid(geteuid())))//��Ч�û�id-->�û���
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
    /*����ҵ����ΪҪ��ӡ���ļ����������֪���ȳ��������������ɵĳ��ȣ�
    �����ֵĿ�ͷ���ֽضϣ���ʡ�Ժ�ȡ��*/
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
     һ�ζ�ȡ�ļ��е�IOBUFSZ���ȣ������͸��ػ�����
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

	if (res.retcode != 0)//����ʧ��
    {
		printf("rejected: %s\n", res.msg);
		exit(1);
	} 
    else 
    {
		printf("job ID %ld\n", ntohl(res.jobid));//�ػ����̷��ص�id������ȡ����ӡ��ҵ
	}

	exit(0);
}
/*
һ���ɹ��Ĵ��ػ�����������Ӧ������ζ�Ŵ�ӡ�����Դ�ӡ���ļ���
������ζ���ػ����̳ɹ��ؽ�����뵽��ӡ��ҵ������
*/