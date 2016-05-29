/*21-2 print.h 包含所有需要的头文件*/

#ifndef _PRINT_H
#define _PRINT_H
/*
 * Print server header file.
 */
#include <sys/socket.h>
#include <arpa/inet.h>
#if defined(BSD) || defined(MACOS)
#include <netinet/in.h>
#endif
#include <netdb.h>
#include <errno.h>

#define CONFIG_FILE    "/etc/printer.conf"
#define SPOOLDIR       "/var/spool/printer"
#define JOBFILE        "jobno"  //包含下一个作业号的文件
#define DATADIR        "data"   //需要打印的文件副本保存目录
#define REQDIR         "reqs"   //每个请求的控制信息存储目录

#define FILENMSZ        64
#define FILEPERM        (S_IRUSR|S_IWUSR)   //创建提交打印文件副本时使用的权限--很受限2
#define USERNM_MAX      64
#define JOBNM_MAX       256
#define MSGLEN_MAX      512

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   256
#endif

#define IPP_PORT        631
#define QLEN            10
#define IBUFSZ          512	/* IPP header buffer size */
#define HBUFSZ          512	/* HTTP header buffer size */
#define IOBUFSZ         8192	/* data buffer size */

#ifndef ETIME
#define ETIME ETIMEDOUT         //由于一些平台不定义错误ETIME，因此另外定义一个错误码，使得在这些系统上有意义
#endif

extern int getaddrlist(const char *, const char *, struct addrinfo **);
extern char *get_printserver(void);
extern struct addrinfo *get_printaddr(void);
extern ssize_t tread(int, void *, size_t, unsigned int);
extern ssize_t treadn(int, void *, size_t, unsigned int);
extern int connect_retry(int, const struct sockaddr *, socklen_t);
extern int initserver(int, struct sockaddr *, socklen_t, int);


/*print命令行程序和打印假脱机守护进程之间的协议*/
/*
 * Structure describing a print request.
 */
struct printreq 
{
	long size;					/* size in bytes */
	long flags;					/* see below */
	char usernm[USERNM_MAX];	/* user's name */
	char jobnm[JOBNM_MAX];		/* job's name */
};

/*
 * Request flags.
 */
#define PR_TEXT		0x01	/* treat file as plain text */

/*
 * The response from the spooling daemon to the print command.
 */
struct printresp 
{
	long retcode;				/* 0=success, !0=error code */
	long jobid;					/* job ID */
	char msg[MSGLEN_MAX];		/* error message */
};

#endif /* _PRINT_H */
