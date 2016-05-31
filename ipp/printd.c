/*21-5 打印假脱机守护进程程序*/

/*
 * Print server daemon.
 */
#include "apue.h"
#include "print.h"
#include "ipp.h"
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <pthread.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/uio.h>

/*
 * These are for the HTTP response from the printer.
 http请求状态
 */
#define HTTP_INFO(x)	((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(x) ((x) >= 200 && (x) <= 299)

/*假脱机守护进程使用job和worker_thread结构来跟踪响应打印作业和接收打印请求的线程*/

/*
 * Describes a print job.
 */
struct job 
{
	struct job      *next;		/* next in list */
	struct job      *prev;		/* previous in list */
	long             jobid;		/* job ID */
	struct printreq  req;		/* copy of print request */
};

/*
 * Describes a thread processing a client request.
 */
struct worker_thread 
{
	struct worker_thread  *next;	/* next in list */
	struct worker_thread  *prev;	/* previous in list */
	pthread_t              tid;		/* thread ID */
	int                    sockfd;	/* socket */
};

/*
 * Needed for logging.
 */
int					log_to_stderr = 0;

/*
 * Printer-related stuff.
 */
struct addrinfo		*printer;//打印机网络地址
char					*printer_name;//打印机主机名称
pthread_mutex_t		configlock = PTHREAD_MUTEX_INITIALIZER;//保护对reread访问
int					reread;//表示守护进程需要再次读取配置文件

/*
 * Thread-related stuff.线程双向链表头，用于接受来自客户端的文件
 */
struct worker_thread	*workers;
pthread_mutex_t		workerlock = PTHREAD_MUTEX_INITIALIZER;
sigset_t				mask;

/*
 * Job-related stuff.挂起作业链表
 */
struct job				*jobhead, *jobtail;
int					jobfd;
long					nextjob;
pthread_mutex_t		joblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t			jobwait = PTHREAD_COND_INITIALIZER;

/*
 * Function prototypes.
 */
void		init_request(void);
void		init_printer(void);
void		update_jobno(void);
long		get_newjobno(void);
void		add_job(struct printreq *, long);
void		replace_job(struct job *);
void		remove_job(struct job *);
void		build_qonstart(void);
void		*client_thread(void *);
void		*printer_thread(void *);
void		*signal_thread(void *);
ssize_t	readmore(int, char **, int, int *);
int		printer_status(int, struct job *);
void		add_worker(pthread_t, int);
void		kill_workers(void);
void		client_cleanup(void *);

/*
 * Main print server thread.  Accepts connect requests from
 * clients and spawns additional threads to service requests.
 *main做两件事情：初始化守护进程+处理来自客户端的请求
 * LOCKING: none.
 */
int main(int argc, char *argv[])
{
	pthread_t			tid;
	struct addrinfo		*ailist = NULL, *aip = NULL;
	int					sockfd = 0, err = 0, i = 0, n = 0, maxfd = 0;
	char				*host = NULL;
	fd_set				rendezvous, rset;
	struct sigaction	sa;
	struct passwd		*pwdp = NULL;

    if (argc != 1)
    {
        err_quit("usage: printd");
    }

    memset(&sa, 0, sizeof(sa));

	daemonize("printd");

    /*忽略SIGPIPE信号
    将要写socket fd，并且不想让写错误出发SIGPIPE，因为默认动作是杀死进程*/
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        log_sys("sigaction failed");
    }

    /*设置线程的信号掩码，创建的所有线程均继承这个信号掩码*/
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);   //告诉守护进程再次读取配置文件
	sigaddset(&mask, SIGTERM);  //告诉守护进程清执行清理工作并优雅退出
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        log_sys("pthread_sigmask failed");
    }

    /*初始化作业请求并确保只有一个守护进程的副本在运行*/
	init_request();

    /*初始化打印机信息*/
	init_printer();

#ifdef _SC_HOST_NAME_MAX
	n = sysconf(_SC_HOST_NAME_MAX);
	if (n < 0)	/* best guess */
#endif
    n = HOST_NAME_MAX;

    if (NULL == (host = malloc(n)))
    {
        log_sys("malloc error");
    }

    if (gethostname(host, n) < 0)
    {
        log_sys("gethostname error");
    }

	if ((err = getaddrlist(host, "print", &ailist)) != 0) 
    {
		log_quit("getaddrinfo error: %s", gai_strerror(err));
		exit(1);
	}

    /*初始化服务器 和 被select的fd*/
	FD_ZERO(&rendezvous);
	maxfd = -1;
	for (aip = ailist; aip != NULL; aip = aip->ai_next) 
    {
		if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, \
		  aip->ai_addrlen, QLEN)) >= 0) 
        {
			FD_SET(sockfd, &rendezvous);
            if (sockfd > maxfd)
            {
                maxfd = sockfd;
            }
		}
	}
    if (-1 == maxfd)
    {
        log_quit("service not enabled");
    }

    /*降低权限，最小特权原则，以避免在守护进程程序中将系统暴露给任何可能的攻击*/
	pwdp = getpwnam("lp");
    if (NULL == pwdp)
    {
        log_sys("can't find user lp");
    }
    if (0 == pwdp->pw_uid)
    {
        log_quit("user lp is privileged");
    }
    if (setuid(pwdp->pw_uid) < 0)
    {
        log_sys("can't change IDs to user lp");
    }

    /*创建处理信号的线程*/
	pthread_create(&tid, NULL, printer_thread, NULL);

    /*创建于打印机通信的线程*/
	pthread_create(&tid, NULL, signal_thread, NULL);

    /*在/var/spool/printer目录中搜索任何挂起的作业*/
	build_qonstart();

    /*完成守护进程的所有初始化工作，记入日志*/
	log_msg("daemon initialized");

	for (;;) 
    {
        /*不能直接使用rendezvous，select会修改入口参数*/
		rset = rendezvous;
        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0)
        {
            log_sys("select failed");
        }

        /*接收到一个连接请求，就accept之，并创建子线程处理客户端请求*/
		for (i = 0; i <= maxfd; i++) 
        {
			if (FD_ISSET(i, &rset)) 
            {

				/*
				 * Accept the connection and handle
				 * the request.
				 */
				sockfd = accept(i, NULL, NULL);

                if (sockfd < 0)
                {
                    log_ret("accept failed");
                }

				pthread_create(&tid, NULL, client_thread, \
				  (void *)sockfd);
			}
		}
	}

	exit(1);
}

/*
 * Initialize the job ID file.  Use a record lock to prevent
 * more than one printer daemon from running at a time.
 *
 * LOCKING: none, except for record-lock on job ID file.
 */
void init_request(void)
{
	int		n = 0;
	char	name[FILENMSZ];

    memset(name, 0, sizeof(name));

	sprintf(name, "%s/%s", SPOOLDIR, JOBFILE);
	jobfd = open(name, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (write_lock(jobfd, 0, SEEK_SET, 0) < 0)
    {
        log_quit("daemon already running");
    }

	/*
	 * Reuse the name buffer for the job counter.
	 */
    if ((n = read(jobfd, name, FILENMSZ)) < 0)
    {
        log_sys("can't read job file");
    }

    if (0 == n)
    {
        nextjob = 1;
    }
    else
    {
        nextjob = atol(name);
    }
}

/*
 * Initialize printer information.
 *
 * LOCKING: none.
 */
void
init_printer(void)
{
	printer = get_printaddr();
	if (printer == NULL) {
		log_msg("no printer device registered");
		exit(1);
	}
	printer_name = printer->ai_canonname;
	if (printer_name == NULL)
		printer_name = "printer";
	log_msg("printer is %s", printer_name);
}

/*
 * Update the job ID file with the next job number.
 *
 * LOCKING: none.
 */
void
update_jobno(void)
{
	char	buf[32];

	lseek(jobfd, 0, SEEK_SET);
	sprintf(buf, "%ld", nextjob);
	if (write(jobfd, buf, strlen(buf)) < 0)
		log_sys("can't update job file");
}

/*
 * Get the next job number.
 *
 * LOCKING: acquires and releases joblock.
 */
long
get_newjobno(void)
{
	long	jobid;

	pthread_mutex_lock(&joblock);
	jobid = nextjob++;
	if (nextjob <= 0)
		nextjob = 1;
	pthread_mutex_unlock(&joblock);
	return(jobid);
}

/*
 * Add a new job to the list of pending jobs.  Then signal
 * the printer thread that a job is pending.
 *
 * LOCKING: acquires and releases joblock.
 */
void
add_job(struct printreq *reqp, long jobid)
{
	struct job	*jp;

	if ((jp = malloc(sizeof(struct job))) == NULL)
		log_sys("malloc failed");
	memcpy(&jp->req, reqp, sizeof(struct printreq));
	jp->jobid = jobid;
	jp->next = NULL;
	pthread_mutex_lock(&joblock);
	jp->prev = jobtail;
	if (jobtail == NULL)
		jobhead = jp;
	else
		jobtail->next = jp;
	jobtail = jp;
	pthread_mutex_unlock(&joblock);
	pthread_cond_signal(&jobwait);
}

/*
 * Replace a job back on the head of the list.
 *
 * LOCKING: acquires and releases joblock.
 */
void
replace_job(struct job *jp)
{
	pthread_mutex_lock(&joblock);
	jp->prev = NULL;
	jp->next = jobhead;
	if (jobhead == NULL)
		jobtail = jp;
	else
		jobhead->prev = jp;
	jobhead = jp;
	pthread_mutex_unlock(&joblock);
}

/*
 * Remove a job from the list of pending jobs.
 *
 * LOCKING: caller must hold joblock.
 */
void
remove_job(struct job *target)
{
	if (target->next != NULL)
		target->next->prev = target->prev;
	else
		jobtail = target->prev;
	if (target->prev != NULL)
		target->prev->next = target->next;
	else
		jobhead = target->next;
}

/*
 * Check the spool directory for pending jobs on start-up.
 *
 * LOCKING: none.
 */
void
build_qonstart(void)
{
	int				fd, err, nr;
	long			jobid;
	DIR				*dirp;
	struct dirent	*entp;
	struct printreq	req;
	char			dname[FILENMSZ], fname[FILENMSZ];

	sprintf(dname, "%s/%s", SPOOLDIR, REQDIR);
	if ((dirp = opendir(dname)) == NULL)
		return;
	while ((entp = readdir(dirp)) != NULL) {
		/*
		 * Skip "." and ".."
		 */
		if (strcmp(entp->d_name, ".") == 0 ||
		  strcmp(entp->d_name, "..") == 0)
			continue;

		/*
		 * Read the request structure.
		 */
		sprintf(fname, "%s/%s/%s", SPOOLDIR, REQDIR, entp->d_name);
		if ((fd = open(fname, O_RDONLY)) < 0)
			continue;
		nr = read(fd, &req, sizeof(struct printreq));
		if (nr != sizeof(struct printreq)) {
			if (nr < 0)
				err = errno;
			else
				err = EIO;
			close(fd);
			log_msg("build_qonstart: can't read %s: %s",
			  fname, strerror(err));
			unlink(fname);
			sprintf(fname, "%s/%s/%s", SPOOLDIR, DATADIR,
			  entp->d_name);
			unlink(fname);
			continue;
		}
		jobid = atol(entp->d_name);
		log_msg("adding job %ld to queue", jobid);
		add_job(&req, jobid);
	}
	closedir(dirp);
}

/*
 * Accept a print job from a client.
 *
 * LOCKING: none.
 */
void *
client_thread(void *arg)
{
	int					n, fd, sockfd, nr, nw, first;
	long				jobid;
	pthread_t			tid;
	struct printreq		req;
	struct printresp	res;
	char				name[FILENMSZ];
	char				buf[IOBUFSZ];

	tid = pthread_self();
	pthread_cleanup_push(client_cleanup, (void *)tid);
	sockfd = (int)arg;
	add_worker(tid, sockfd);

	/*
	 * Read the request header.
	 */
	if ((n = treadn(sockfd, &req, sizeof(struct printreq), 10)) !=
	  sizeof(struct printreq)) {
		res.jobid = 0;
		if (n < 0)
			res.retcode = htonl(errno);
		else
			res.retcode = htonl(EIO);
		strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
		writen(sockfd, &res, sizeof(struct printresp));
		pthread_exit((void *)1);
	}
	req.size = ntohl(req.size);
	req.flags = ntohl(req.flags);

	/*
	 * Create the data file.
	 */
	jobid = get_newjobno();
	sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);
	if ((fd = creat(name, FILEPERM)) < 0) {
		res.jobid = 0;
		if (n < 0)
			res.retcode = htonl(errno);
		else
			res.retcode = htonl(EIO);
		log_msg("client_thread: can't create %s: %s", name,
		  strerror(res.retcode));
		strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
		writen(sockfd, &res, sizeof(struct printresp));
		pthread_exit((void *)1);
	}

	/*
	 * Read the file and store it in the spool directory.
	 */
	first = 1;
	while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0) {
		if (first) {
			first = 0;
			if (strncmp(buf, "%!PS", 4) != 0)
				req.flags |= PR_TEXT;
		}
		nw = write(fd, buf, nr);
		if (nw != nr) {
			if (nw < 0)
				res.retcode = htonl(errno);
			else
				res.retcode = htonl(EIO);
			log_msg("client_thread: can't write %s: %s", name,
			  strerror(res.retcode));
			close(fd);
			strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
			writen(sockfd, &res, sizeof(struct printresp));
			unlink(name);
			pthread_exit((void *)1);
		}
	}
	close(fd);

	/*
	 * Create the control file.
	 */
	sprintf(name, "%s/%s/%ld", SPOOLDIR, REQDIR, jobid);
	fd = creat(name, FILEPERM);
	if (fd < 0) {
		res.jobid = 0;
		if (n < 0)
			res.retcode = htonl(errno);
		else
			res.retcode = htonl(EIO);
		log_msg("client_thread: can't create %s: %s", name,
		  strerror(res.retcode));
		strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
		writen(sockfd, &res, sizeof(struct printresp));
		sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);
		unlink(name);
		pthread_exit((void *)1);
	}
	nw = write(fd, &req, sizeof(struct printreq));
	if (nw != sizeof(struct printreq)) {
		res.jobid = 0;
		if (nw < 0)
			res.retcode = htonl(errno);
		else
			res.retcode = htonl(EIO);
		log_msg("client_thread: can't write %s: %s", name,
		  strerror(res.retcode));
		close(fd);
		strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
		writen(sockfd, &res, sizeof(struct printresp));
		unlink(name);
		sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);
		unlink(name);
		pthread_exit((void *)1);
	}
	close(fd);

	/*
	 * Send response to client.
	 */
	res.retcode = 0;
	res.jobid = htonl(jobid);
	sprintf(res.msg, "request ID %ld", jobid);
	writen(sockfd, &res, sizeof(struct printresp));

	/*
	 * Notify the printer thread, clean up, and exit.
	 */
	log_msg("adding job %ld to queue", jobid);
	add_job(&req, jobid);
	pthread_cleanup_pop(1);
	return((void *)0);
}

/*
* Add a worker to the list of worker threads.
*
* LOCKING: acquires and releases workerlock.
*/
void
add_worker(pthread_t tid, int sockfd)
{
	struct worker_thread	*wtp;

	if ((wtp = malloc(sizeof(struct worker_thread))) == NULL) {
		log_ret("add_worker: can't malloc");
		pthread_exit((void *)1);
	}
	wtp->tid = tid;
	wtp->sockfd = sockfd;
	pthread_mutex_lock(&workerlock);
	wtp->prev = NULL;
	wtp->next = workers;
	if (workers == NULL)
		workers = wtp;
	else
		workers->prev = wtp;
	pthread_mutex_unlock(&workerlock);
}

/*
 * Cancel (kill) all outstanding workers.
 *
 * LOCKING: acquires and releases workerlock.
 */
void
kill_workers(void)
{
	struct worker_thread	*wtp;

	pthread_mutex_lock(&workerlock);
	for (wtp = workers; wtp != NULL; wtp = wtp->next)
		pthread_cancel(wtp->tid);
	pthread_mutex_unlock(&workerlock);
}

/*
 * Cancellation routine for the worker thread.
 *
 * LOCKING: acquires and releases workerlock.
 */
void
client_cleanup(void *arg)
{
	struct worker_thread	*wtp;
	pthread_t				tid;

	tid = (pthread_t)arg;
	pthread_mutex_lock(&workerlock);
	for (wtp = workers; wtp != NULL; wtp = wtp->next) {
		if (wtp->tid == tid) {
			if (wtp->next != NULL)
				wtp->next->prev = wtp->prev;
			if (wtp->prev != NULL)
				wtp->prev->next = wtp->next;
			else
				workers = wtp->next;
			break;
		}
	}
	pthread_mutex_unlock(&workerlock);
	if (wtp != NULL) {
		close(wtp->sockfd);
		free(wtp);
	}
}

/*
 * Deal with signals.
 *
 * LOCKING: acquires and releases configlock.
 */
void *
signal_thread(void *arg)
{
	int		err, signo;

	for (;;) {
		err = sigwait(&mask, &signo);
		if (err != 0)
			log_quit("sigwait failed: %s", strerror(err));
		switch (signo) {
		case SIGHUP:
			/*
			 * Schedule to re-read the configuration file.
			 */
			pthread_mutex_lock(&configlock);
			reread = 1;
			pthread_mutex_unlock(&configlock);
			break;

		case SIGTERM:
			kill_workers();
			log_msg("terminate with signal %s", strsignal(signo));
			exit(0);

		default:
			kill_workers();
			log_quit("unexpected signal %d", signo);
		}
	}
}

/*
 * Add an option to the IPP header.
 *
 * LOCKING: none.
 */
char *
add_option(char *cp, int tag, char *optname, char *optval)
{
	int		n;
	union {
		int16_t s;
		char c[2];
	}		u;

	*cp++ = tag;
	n = strlen(optname);
	u.s = htons(n);
	*cp++ = u.c[0];
	*cp++ = u.c[1];
	strcpy(cp, optname);
	cp += n;
	n = strlen(optval);
	u.s = htons(n);
	*cp++ = u.c[0];
	*cp++ = u.c[1];
	strcpy(cp, optval);
	return(cp + n);
}

/*
 * Single thread to communicate with the printer.
 *
 * LOCKING: acquires and releases joblock and configlock.
 */
void *
printer_thread(void *arg)
{
	struct job		*jp;
	int				hlen, ilen, sockfd, fd, nr, nw;
	char			*icp, *hcp;
	struct ipp_hdr	*hp;
	struct stat		sbuf;
	struct iovec	iov[2];
	char			name[FILENMSZ];
	char			hbuf[HBUFSZ];
	char			ibuf[IBUFSZ];
	char			buf[IOBUFSZ];
	char			str[64];

	for (;;) {
		/*
		 * Get a job to print.
		 */
		pthread_mutex_lock(&joblock);
		while (jobhead == NULL) {
			log_msg("printer_thread: waiting...");
			pthread_cond_wait(&jobwait, &joblock);
		}
		remove_job(jp = jobhead);
		log_msg("printer_thread: picked up job %ld", jp->jobid);
		pthread_mutex_unlock(&joblock);

		update_jobno();

		/*
		 * Check for a change in the config file.
		 */
		pthread_mutex_lock(&configlock);
		if (reread) {
			freeaddrinfo(printer);
			printer = NULL;
			printer_name = NULL;
			reread = 0;
			pthread_mutex_unlock(&configlock);
			init_printer();
		} else {
			pthread_mutex_unlock(&configlock);
		}

		/*
		 * Send job to printer.
		 */
		sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jp->jobid);
		if ((fd = open(name, O_RDONLY)) < 0) {
			log_msg("job %ld canceled - can't open %s: %s",
			  jp->jobid, name, strerror(errno));
			free(jp);
			continue;
		}
		if (fstat(fd, &sbuf) < 0) {
			log_msg("job %ld canceled - can't fstat %s: %s",
			  jp->jobid, name, strerror(errno));
			free(jp);
			close(fd);
			continue;
		}
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			log_msg("job %ld deferred - can't create socket: %s",
			  jp->jobid, strerror(errno));
			goto defer;
		}
		if (connect_retry(sockfd, printer->ai_addr,
		  printer->ai_addrlen) < 0) {
			log_msg("job %ld deferred - can't contact printer: %s",
			  jp->jobid, strerror(errno));
			goto defer;
		}

		/*
		 * Set up the IPP header.
		 */
		icp = ibuf;
		hp = (struct ipp_hdr *)icp;
		hp->major_version = 1;
		hp->minor_version = 1;
		hp->operation = htons(OP_PRINT_JOB);
		hp->request_id = htonl(jp->jobid);
		icp += offsetof(struct ipp_hdr, attr_group);
		*icp++ = TAG_OPERATION_ATTR;
		icp = add_option(icp, TAG_CHARSET, "attributes-charset",
		  "utf-8");
		icp = add_option(icp, TAG_NATULANG,
		  "attributes-natural-language", "en-us");
		sprintf(str, "http://%s:%d", printer_name, IPP_PORT);
		icp = add_option(icp, TAG_URI, "printer-uri", str);
		icp = add_option(icp, TAG_NAMEWOLANG,
		  "requesting-user-name", jp->req.usernm);
		icp = add_option(icp, TAG_NAMEWOLANG, "job-name",
		  jp->req.jobnm);
		if (jp->req.flags & PR_TEXT) {
			icp = add_option(icp, TAG_MIMETYPE, "document-format",
			  "text/plain");
		} else {
			icp = add_option(icp, TAG_MIMETYPE, "document-format",
			  "application/postscript");
		}
		*icp++ = TAG_END_OF_ATTR;
		ilen = icp - ibuf;

		/*
		 * Set up the HTTP header.
		 */
		hcp = hbuf;
		sprintf(hcp, "POST /%s/ipp HTTP/1.1\r\n", printer_name);
		hcp += strlen(hcp);
		sprintf(hcp, "Content-Length: %ld\r\n",
		  (long)sbuf.st_size + ilen);
		hcp += strlen(hcp);
		strcpy(hcp, "Content-Type: application/ipp\r\n");
		hcp += strlen(hcp);
		sprintf(hcp, "Host: %s:%d\r\n", printer_name, IPP_PORT);
		hcp += strlen(hcp);
		*hcp++ = '\r';
		*hcp++ = '\n';
		hlen = hcp - hbuf;

		/*
		 * Write the headers first.  Then send the file.
		 */
		iov[0].iov_base = hbuf;
		iov[0].iov_len = hlen;
		iov[1].iov_base = ibuf;
		iov[1].iov_len = ilen;
		if ((nw = writev(sockfd, iov, 2)) != hlen + ilen) {
			log_ret("can't write to printer");
			goto defer;
		}
		while ((nr = read(fd, buf, IOBUFSZ)) > 0) {
			if ((nw = write(sockfd, buf, nr)) != nr) {
				if (nw < 0)
				  log_ret("can't write to printer");
				else
				  log_msg("short write (%d/%d) to printer", nw, nr);
				goto defer;
			}
		}
		if (nr < 0) {
			log_ret("can't read %s", name);
			goto defer;
		}

		/*
		 * Read the response from the printer.
		 */
		if (printer_status(sockfd, jp)) {
			unlink(name);
			sprintf(name, "%s/%s/%ld", SPOOLDIR, REQDIR, jp->jobid);
			unlink(name);
			free(jp);
			jp = NULL;
		}
defer:
		close(fd);
		if (sockfd >= 0)
			close(sockfd);
		if (jp != NULL) {
			replace_job(jp);
			sleep(60);
		}
	}
}

/*
 * Read data from the printer, possibly increasing the buffer.
 * Returns offset of end of data in buffer or -1 on failure.
 *
 * LOCKING: none.
 */
ssize_t
readmore(int sockfd, char **bpp, int off, int *bszp)
{
	ssize_t	nr;
	char	*bp = *bpp;
	int		bsz = *bszp;

	if (off >= bsz) {
		bsz += IOBUFSZ;
		if ((bp = realloc(*bpp, bsz)) == NULL)
			log_sys("readmore: can't allocate bigger read buffer");
		*bszp = bsz;
		*bpp = bp;
	}
	if ((nr = tread(sockfd, &bp[off], bsz-off, 1)) > 0)
		return(off+nr);
	else
		return(-1);
}

/*
 * Read and parse the response from the printer.  Return 1
 * if the request was successful, and 0 otherwise.
 *
 * LOCKING: none.
 */
int
printer_status(int sockfd, struct job *jp)
{
	int				i, success, code, len, found, bufsz;
	long			jobid;
	ssize_t			nr;
	char			*statcode, *reason, *cp, *contentlen;
	struct ipp_hdr	*hp;
	char			*bp;

	/*
	 * Read the HTTP header followed by the IPP response header.
	 * They can be returned in multiple read attempts.  Use the
	 * Content-Length specifier to determine how much to read.
	 */
	success = 0;
	bufsz = IOBUFSZ;
	if ((bp = malloc(IOBUFSZ)) == NULL)
		log_sys("printer_status: can't allocate read buffer");

	while ((nr = tread(sockfd, bp, IOBUFSZ, 5)) > 0) {
		/*
		 * Find the status.  Response starts with "HTTP/x.y"
		 * so we can skip the first 8 characters.
		 */
		cp = bp + 8;
		while (isspace((int)*cp))
			cp++;
		statcode = cp;
		while (isdigit((int)*cp))
			cp++;
		if (cp == statcode) { /* Bad format; log it and move on */
			log_msg(bp);
		} else {
			*cp++ = '\0';
			reason = cp;
			while (*cp != '\r' && *cp != '\n')
				cp++;
			*cp = '\0';
			code = atoi(statcode);
			if (HTTP_INFO(code))
				continue;
			if (!HTTP_SUCCESS(code)) { /* probable error: log it */
				bp[nr] = '\0';
				log_msg("error: %s", reason);
				break;
			}

			/*
			 * The HTTP request was okay, but we still
			 * need to check the IPP status.  First
			 * search for the Content-Length specifier.
			 */
			i = cp - bp;
			for (;;) {
				while (*cp != 'C' && *cp != 'c' && i < nr) {
					cp++;
					i++;
				}
				if (i >= nr &&	/* get more header */
				  ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
					goto out;
				cp = &bp[i];
				if (strncasecmp(cp, "Content-Length:", 15) == 0) {
					cp += 15;
					while (isspace((int)*cp))
						cp++;
					contentlen = cp;
					while (isdigit((int)*cp))
						cp++;
					*cp++ = '\0';
					i = cp - bp;
					len = atoi(contentlen);
					break;
				} else {
					cp++;
					i++;
				}
			}
			if (i >= nr &&	/* get more header */
			  ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
				goto out;
			cp = &bp[i];

			found = 0;
			while (!found) {  /* look for end of HTTP header */
				while (i < nr - 2) {
					if (*cp == '\n' && *(cp + 1) == '\r' &&
					  *(cp + 2) == '\n') {
						found = 1;
						cp += 3;
						i += 3;
						break;
					}
					cp++;
					i++;
				}
				if (i >= nr &&	/* get more header */
				  ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
					goto out;
				cp = &bp[i];
			}

			if (nr - i < len &&	/* get more header */
			  ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
				goto out;
			cp = &bp[i];

			hp = (struct ipp_hdr *)cp;
			i = ntohs(hp->Status);
			jobid = ntohl(hp->request_id);
			if (jobid != jp->jobid) {
				/*
				 * Different jobs.  Ignore it.
				 */
				log_msg("jobid %ld status code %d", jobid, i);
				break;
			}

			if (STATCLASS_OK(i))
				success = 1;
			break;
		}
	}

out:
	free(bp);
	if (nr < 0) {
		log_msg("jobid %ld: error reading printer response: %s",
		  jobid, strerror(errno));
	}
	return(success);
}
