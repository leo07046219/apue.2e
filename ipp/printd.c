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
#define HTTP_INFO(x)    ((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(x) ((x) >= 200 && (x) <= 299)

  /*假脱机守护进程使用job和worker_thread结构来跟踪响应打印作业和接收打印请求的线程*/

  /*
   * Describes a print job.
   */
struct job
{
    struct job      *next;        /* next in list */
    struct job      *prev;        /* previous in list */
    long             jobid;        /* job ID */
    struct printreq  req;        /* copy of print request */
};

/*
 * Describes a thread processing a client request.
 */
struct worker_thread
{
    struct worker_thread  *next;    /* next in list */
    struct worker_thread  *prev;    /* previous in list */
    pthread_t              tid;        /* thread ID */
    int                    sockfd;    /* socket */
};

/*
 * Needed for logging.
 */
int                    log_to_stderr = 0;

/*
 * Printer-related stuff.
 */
struct addrinfo        *printer;//打印机网络地址
char                    *printer_name;//打印机主机名称
pthread_mutex_t        configlock = PTHREAD_MUTEX_INITIALIZER;//保护对reread访问
int                    reread;//表示守护进程需要再次读取配置文件

/*
 * Thread-related stuff.线程双向链表头，用于接受来自客户端的文件
 */
struct worker_thread    *workers;
pthread_mutex_t        workerlock = PTHREAD_MUTEX_INITIALIZER;
sigset_t                mask;

/*
 * Job-related stuff.挂起作业链表
 */
struct job                *jobhead, *jobtail;
int                    jobfd;
long                    nextjob;
pthread_mutex_t        joblock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t            jobwait = PTHREAD_COND_INITIALIZER;

/*
 * Function prototypes.
 */
void        init_request(void);
void        init_printer(void);
void        update_jobno(void);
long        get_newjobno(void);
void        add_job(struct printreq *, long);
void        replace_job(struct job *);
void        remove_job(struct job *);
void        build_qonstart(void);
void        *client_thread(void *);
void        *printer_thread(void *);
void        *signal_thread(void *);
ssize_t    readmore(int, char **, int, int *);
int        printer_status(int, struct job *);
void        add_worker(pthread_t, int);
void        kill_workers(void);
void        client_cleanup(void *);

/*
 * Main print server thread.  Accepts connect requests from
 * clients and spawns additional threads to service requests.
 *main做两件事情：初始化守护进程+处理来自客户端的请求
 * LOCKING: none.
 */
int main(int argc, char *argv[])
{
    pthread_t            tid;
    struct addrinfo        *ailist = NULL, *aip = NULL;
    int                    sockfd = 0, err = 0, i = 0, n = 0, maxfd = 0;
    char                *host = NULL;
    fd_set                rendezvous, rset;
    struct sigaction    sa;
    struct passwd        *pwdp = NULL;

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
    if (n < 0)    /* best guess */
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
    int        n = 0;
    char    name[FILENMSZ];

    memset(name, 0, sizeof(name));

    /*利用文件锁保证守护进程的唯一性*/
    sprintf(name, "%s/%s", SPOOLDIR, JOBFILE);
    jobfd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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

    /*若文件刚创建并因此为空，那么将nextjob设置为1*/
    if (0 == n)
    {
        nextjob = 1;
    }
    else
    {
        nextjob = atol(name);
    }
    /*不能关闭该文件，因为这将释放已经放置在上面的写锁*/
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
void update_jobno(void)
{
    char    buf[32];

    memset(buf, 0, sizeof(buf));

    lseek(jobfd, 0, SEEK_SET);

    sprintf(buf, "%ld", nextjob);

    if (write(jobfd, buf, strlen(buf)) < 0)
    {
        log_sys("can't update job file");
    }
}

/*
 * Get the next job number.
 *
 * LOCKING: acquires and releases joblock.
 */
long get_newjobno(void)
{
    long    jobid = 0;

    pthread_mutex_lock(&joblock);

    jobid = nextjob++;
    if (nextjob <= 0)
    {
        nextjob = 1;
    }

    pthread_mutex_unlock(&joblock);

    return(jobid);
}

/*
 * Add a new job to the list of pending jobs.  Then signal
 * the printer thread that a job is pending.
 *
 * LOCKING: acquires and releases joblock.
 */
void add_job(struct printreq *reqp, long jobid)
{
    struct job    *jp = NULL;

    if (NULL == (jp = malloc(sizeof(struct job))))
    {
        log_sys("malloc failed");
    }

    memcpy(&jp->req, reqp, sizeof(struct printreq));
    jp->jobid = jobid;
    jp->next = NULL;

    pthread_mutex_lock(&joblock);
    jp->prev = jobtail;
    if (NULL == jobtail)
    {
        jobhead = jp;
    }
    else
    {
        jobtail->next = jp;
    }
    jobtail = jp;
    pthread_mutex_unlock(&joblock);

    /*给打印机线程发信号，告诉该线程另一个作业可用了*/
    pthread_cond_signal(&jobwait);
}

/*
 * Replace a job back on the head of the list.
 *将作业插入到挂起作业列表头部
 * LOCKING: acquires and releases joblock.
 */
void replace_job(struct job *jp)
{
    pthread_mutex_lock(&joblock);
    jp->prev = NULL;
    jp->next = jobhead;
    if (NULL == jobhead)
    {
        jobtail = jp;
    }
    else
    {
        jobhead->prev = jp;
    }
    jobhead = jp;
    pthread_mutex_unlock(&joblock);
}

/*
 * Remove a job from the list of pending jobs.
 *
 * LOCKING: caller must hold joblock.
 */
void remove_job(struct job *target)
{
    if (target->next != NULL)
    {
        target->next->prev = target->prev;
    }
    else
    {
        jobtail = target->prev;
    }
    if (target->prev != NULL)
    {
        target->prev->next = target->next;
    }
    else
    {
        jobhead = target->next;
    }
}

/*
 * Check the spool directory for pending jobs on start-up.
 *从存储在/var/spool/printer/reqs中的磁盘文件建立一个内存中的打印作业列表
 * LOCKING: none.
 */
void build_qonstart(void)
{
    int                fd = 0, err = 0, nr = 0;
    long            jobid = 0;
    DIR                *dirp = NULL;
    struct dirent    *entp = NULL;
    struct printreq    req;
    char            dname[FILENMSZ], fname[FILENMSZ];

    memset(&req, 0, sizeof(req));
    memset(dname, 0, sizeof(dname));
    memset(fname, 0, sizeof(fname));

    sprintf(dname, "%s/%s", SPOOLDIR, REQDIR);
    if (NULL == (dirp = opendir(dname)))
    {
        return;
    }
    /*遍历目录从除.和..之外的所有条目，读取保存在文件中的printreq结构*/
    while ((entp = readdir(dirp)) != NULL)
    {
        /*
         * Skip "." and ".."
         */
        if (strcmp(entp->d_name, ".") == 0 || \
            strcmp(entp->d_name, "..") == 0)
        {
            continue;
        }

        /*
         * Read the request structure.
         */
        sprintf(fname, "%s/%s/%s", SPOOLDIR, REQDIR, entp->d_name);
        if ((fd = open(fname, O_RDONLY)) < 0)
        {
            continue;
        }

        nr = read(fd, &req, sizeof(struct printreq));
        if (nr != sizeof(struct printreq))
        {
            if (nr < 0)
            {
                err = errno;
            }
            else
            {
                err = EIO;
            }
            close(fd);
            log_msg("build_qonstart: can't read %s: %s", \
                fname, strerror(err));
            unlink(fname);
            sprintf(fname, "%s/%s/%s", SPOOLDIR, DATADIR, \
                entp->d_name);
            unlink(fname);
            continue;
        }
        /*将文件名转换为作业id*/
        jobid = atol(entp->d_name);
        log_msg("adding job %ld to queue", jobid);
        add_job(&req, jobid);
    }

    closedir(dirp);
}

/*
 * Accept a print job from a client.
 *从客户端print命令中接收要打印的文件
 * LOCKING: none.
 */
void *client_thread(void *arg)
{
    int                    n = 0, fd = 0, sockfd = 0, nr = 0, nw = 0, first = 0;
    long                jobid = 0;
    pthread_t            tid;
    struct printreq        req;
    struct printresp    res;
    char                name[FILENMSZ];
    char                buf[IOBUFSZ];

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    memset(name, 0, sizeof(name));
    memset(buf, 0, sizeof(buf));

    /*安装线程清理处理程序*/
    tid = pthread_self();
    pthread_cleanup_push(client_cleanup, (void *)tid);
    sockfd = (int)arg;
    add_worker(tid, sockfd);

    /*
     * Read the request header.
     */
    if ((n = treadn(sockfd, &req, sizeof(struct printreq), 10)) != \
        sizeof(struct printreq))
    {
        /*接收数据不对，向客户端返回错误号，并退出线程 --- 可以提取函数,多个地方用到*/
        res.jobid = 0;
        if (n < 0)
        {
            res.retcode = htonl(errno);
        }
        else
        {
            res.retcode = htonl(EIO);
        }
        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        /*不需要显示关闭sockfd，但调用pthread_exit时，线程清理处理程序会处理文件描述符*/

        pthread_exit((void *)1);
    }

    req.size = ntohl(req.size);
    req.flags = ntohl(req.flags);

    /*
     * Create the data file.
     */
    jobid = get_newjobno();
    sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);

    if ((fd = creat(name, FILEPERM)) < 0)
    {
        res.jobid = 0;
        if (n < 0)
        {
            res.retcode = htonl(errno);
        }
        else
        {
            res.retcode = htonl(EIO);
        }

        log_msg("client_thread: can't create %s: %s", name, \
            strerror(res.retcode));

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));

        pthread_exit((void *)1);
    }

    /*
     * Read the file and store it in the spool directory.保存数据文件
     */
    first = 1;
    while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0)
    {
        if (first)
        {
            first = 0;
            if (strncmp(buf, "%!PS", 4) != 0)   //PostScript是以%!PS模式开头的
            {
                req.flags |= PR_TEXT;   //纯文本文件
            }
        }

        nw = write(fd, buf, nr);
        if (nw != nr)
        {
            if (nw < 0)
            {
                res.retcode = htonl(errno);
            }
            else
            {
                res.retcode = htonl(EIO);
            }

            log_msg("client_thread: can't write %s: %s", name, \
                strerror(res.retcode));
            close(fd);
            strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
            writen(sockfd, &res, sizeof(struct printresp));
            /*不需要显示关闭sockfd，但调用pthread_exit时，线程清理处理程序会处理文件描述符*/
            unlink(name);

            pthread_exit((void *)1);
        }
    }
    close(fd);

    /*
     * Create the control file.记住打印请求，以作业id为文件名
     */
    sprintf(name, "%s/%s/%ld", SPOOLDIR, REQDIR, jobid);
    fd = creat(name, FILEPERM);
    if (fd < 0)
    {
        res.jobid = 0;
        if (n < 0)
        {
            res.retcode = htonl(errno);
        }
        else
        {
            res.retcode = htonl(EIO);
        }
        log_msg("client_thread: can't create %s: %s", name, \
            strerror(res.retcode));

        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);
        unlink(name);

        pthread_exit((void *)1);
    }

    nw = write(fd, &req, sizeof(struct printreq));
    if (nw != sizeof(struct printreq))
    {
        res.jobid = 0;
        if (nw < 0)
        {
            res.retcode = htonl(errno);
        }
        else
        {
            res.retcode = htonl(EIO);
        }
        log_msg("client_thread: can't write %s: %s", name, \
            strerror(res.retcode));
        close(fd);
        strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
        writen(sockfd, &res, sizeof(struct printresp));
        unlink(name);
        sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jobid);
        unlink(name);
        pthread_exit((void *)1);
    }
    /*关闭控制文件描述符
    和进程不同，当一个线程结束并且进程中有其他线程时，文件描述符不会自动关闭，
    如果不关闭不需要的文件描述符，终将耗尽资源*/
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
*将一个worker_thread结构加入到活动线程列表中
* LOCKING: acquires and releases workerlock.
*/
void add_worker(pthread_t tid, int sockfd)
{
    struct worker_thread    *wtp = NULL;

    /*client_cleanup()中free*/
    if ((wtp = malloc(sizeof(struct worker_thread))) == NULL)
    {
        log_ret("add_worker: can't malloc");
        pthread_exit((void *)1);
    }
    wtp->tid = tid;
    wtp->sockfd = sockfd;

    /*将结构加到列表头部*/
    pthread_mutex_lock(&workerlock);
    wtp->prev = NULL;
    wtp->next = workers;
    if (NULL == workers)
    {
        workers = wtp;
    }
    else
    {
        workers->prev = wtp;
    }
    pthread_mutex_unlock(&workerlock);
}

/*
 * Cancel (kill) all outstanding workers.
 *
 * LOCKING: acquires and releases workerlock.
 */
void kill_workers(void)
{
    struct worker_thread    *wtp = NULL;

    pthread_mutex_lock(&workerlock);

    for (wtp = workers; wtp != NULL; wtp = wtp->next)
    {
        /*实际的删除动作在每个线程到达实际的删除点时发生 --- 线程安全退出机制*/
        pthread_cancel(wtp->tid);
    }

    pthread_mutex_unlock(&workerlock);
}

/*
 * Cancellation routine for the worker thread.
 *与客户端命令通信的工作者线程的线程清理处理程序，
 用非零参数调用pthread_cleanup_pop，或者响应一个删除请求时，该函数被调用。
 * LOCKING: acquires and releases workerlock.
 */
void client_cleanup(void *arg)
{
    struct worker_thread    *wtp = NULL;
    pthread_t                tid;

    tid = (pthread_t)arg;

    pthread_mutex_lock(&workerlock);
    for (wtp = workers; wtp != NULL; wtp = wtp->next)
    {
        if (wtp->tid == tid)
        {
            if (wtp->next != NULL)
            {
                wtp->next->prev = wtp->prev;
            }
            if (wtp->prev != NULL)
            {
                wtp->prev->next = wtp->next;
            }
            else
            {
                workers = wtp->next;
            }

            break;
        }
    }
    pthread_mutex_unlock(&workerlock);

    /*释放之前分配的内存*/
    if (wtp != NULL)
    {
        close(wtp->sockfd);
        free(wtp);
    }
}

/*
 * Deal with signals.
 *由负责信号处理的线程运行
 * LOCKING: acquires and releases configlock.
 */
void *signal_thread(void *arg)
{
    int        err = 0, signo = 0;

    for (;;)
    {
        /*等待信号中的一个出现*/
        err = sigwait(&mask, &signo);

        if (err != 0)
        {
            log_quit("sigwait failed: %s", strerror(err));
        }

        switch (signo)
        {
        case SIGHUP:
            /*
             * Schedule to re-read the configuration file.
             */
            pthread_mutex_lock(&configlock);
            reread = 1;
            /*打印守护进程程序printer_thread中根据reread标志位决定是否重读配置文件*/
            pthread_mutex_unlock(&configlock);
            break;

        case SIGTERM:
            /*杀死所有工作线程*/
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
 *在送往打印机的IPP首部添加选项
 * LOCKING: none.
 */
char * add_option(char *cp, int tag, char *optname, char *optval)
{
    int        n = 0;

    /*一些处理器架构，例如SPARC，并不能从任意地址装入一个整数，
    通过下面方式实现：
    n转转成网络字节序，赋给u.s,将u.c[2]赋给cp*/
    union
    {
        int16_t s;
        char c[2];
    }u;

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

    /*返回首部中下一部分应该开始的地址*/
    return(cp + n);
}

/*
 * Single thread to communicate with the printer.
 *
 * LOCKING: acquires and releases joblock and configlock.
 */
void *printer_thread(void *arg)
{
    struct job        *jp = NULL;
    int                hlen = 0, ilen = 0, sockfd = 0, fd = 0, nr = 0, nw = 0;
    char            *icp = NULL, *hcp = NULL;
    struct ipp_hdr    *hp = NULL;
    struct stat        sbuf;
    struct iovec    iov[2];
    char            name[FILENMSZ];
    char            hbuf[HBUFSZ];
    char            ibuf[IBUFSZ];
    char            buf[IOBUFSZ];
    char            str[64];

    memset(&sbuf, 0, sizeof(sbuf));
    memset(iov, 0, sizeof(iov));
    memset(name, 0, sizeof(name));
    memset(hbuf, 0, sizeof(hbuf));
    memset(buf, 0, sizeof(buf));
    memset(str, 0, sizeof(str));

    for (;;)
    {
        /*
         * Get a job to print.
         */
        pthread_mutex_lock(&joblock);
        while (NULL == jobhead)
        {
            log_msg("printer_thread: waiting...");
            pthread_cond_wait(&jobwait, &joblock);
        }
        remove_job(jp = jobhead);
        log_msg("printer_thread: picked up job %ld", jp->jobid);
        pthread_mutex_unlock(&joblock);

        /*将下一个作业号写入到/var/spool/printer/jobno中*/
        update_jobno();

        /*
         * Check for a change in the config file.
         */
        pthread_mutex_lock(&configlock);
        if (reread)
        {
            freeaddrinfo(printer);
            printer = NULL;
            printer_name = NULL;
            reread = 0;
            pthread_mutex_unlock(&configlock);
            init_printer();
        }
        else
        {
            pthread_mutex_unlock(&configlock);
        }

        /*
         * Send job to printer.
         */
        sprintf(name, "%s/%s/%ld", SPOOLDIR, DATADIR, jp->jobid);
        if ((fd = open(name, O_RDONLY)) < 0)
        {
            log_msg("job %ld canceled - can't open %s: %s", \
                jp->jobid, name, strerror(errno));
            free(jp);
            continue;
        }

        if (fstat(fd, &sbuf) < 0)
        {
            log_msg("job %ld canceled - can't fstat %s: %s", \
                jp->jobid, name, strerror(errno));
            free(jp);
            close(fd);
            continue;
        }

        /*建立与打印机的tcp连接*/
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            log_msg("job %ld deferred - can't create socket: %s",
                jp->jobid, strerror(errno));
            goto defer;
        }

        if (connect_retry(sockfd, printer->ai_addr, \
            printer->ai_addrlen) < 0)
        {
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

        icp = add_option(icp, TAG_CHARSET, "attributes-charset", \
            "utf-8");

        icp = add_option(icp, TAG_NATULANG, \
            "attributes-natural-language", "en-us");
        sprintf(str, "http://%s:%d", printer_name, IPP_PORT);

        icp = add_option(icp, TAG_URI, "printer-uri", str);

        icp = add_option(icp, TAG_NAMEWOLANG, \
            "requesting-user-name", jp->req.usernm);

        icp = add_option(icp, TAG_NAMEWOLANG, "job-name", \
            jp->req.jobnm);

        if (jp->req.flags & PR_TEXT)
        {
            icp = add_option(icp, TAG_MIMETYPE, "document-format", \
                "text/plain");
        }
        else
        {
            icp = add_option(icp, TAG_MIMETYPE, "document-format", \
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
        sprintf(hcp, "Content-Length: %ld\r\n", \
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
        if ((nw = writev(sockfd, iov, 2)) != hlen + ilen)
        {
            log_ret("can't write to printer");
            goto defer;
        }

        while ((nr = read(fd, buf, IOBUFSZ)) > 0)
        {
            if ((nw = write(sockfd, buf, nr)) != nr)
            {
                if (nw < 0)
                {
                    log_ret("can't write to printer");
                }
                else
                {
                    log_msg("short write (%d/%d) to printer", nw, nr);
                }
                goto defer;
            }
        }

        if (nr < 0)
        {
            log_ret("can't read %s", name);
            goto defer;
        }

        /*
         * Read the response from the printer.
         */
        if (printer_status(sockfd, jp))
        {
            unlink(name);
            sprintf(name, "%s/%s/%ld", SPOOLDIR, REQDIR, jp->jobid);
            unlink(name);
            free(jp);
            jp = NULL;
        }

    defer:
        /*清理，延迟，try again*/
        close(fd);

        if (sockfd >= 0)
        {
            close(sockfd);
        }

        if (jp != NULL)
        {
            replace_job(jp);
            sleep(60);
        }
    }
}

/*
 * Read data from the printer, possibly increasing the buffer.
 * Returns offset of end of data in buffer or -1 on failure.
 *读取来自打印机的部分响应消息
 * LOCKING: none.
 */
ssize_t readmore(int sockfd, char **bpp, int off, int *bszp)
{
    ssize_t     nr;
    char        *bp = *bpp;
    int         bsz = *bszp;

    if (off >= bsz)
    {
        /*动态调整缓冲区*/
        bsz += IOBUFSZ;
        if (NULL == (bp = realloc(*bpp, bsz)))
        {
            log_sys("readmore: can't allocate bigger read buffer");
        }

        *bszp = bsz;
        *bpp = bp;
    }
    if ((nr = tread(sockfd, &bp[off], bsz - off, 1)) > 0)
    {
        return(off + nr);
    }
    else
    {
        return(-1);
    }
}

/*
 * Read and parse the response from the printer.  Return 1
 * if the request was successful, and 0 otherwise.
 *读取打印机对一个打印作业的请求的的响应消息
 * LOCKING: none.
 */
int printer_status(int sockfd, struct job *jp)
{
    int             i = 0, success = 0, code = 0, len = 0, found = 0, bufsz = 0;
    long            jobid = 0;
    ssize_t         nr;
    char            *statcode = NULL, *reason = NULL, *cp = NULL, *contentlen = NULL;
    struct ipp_hdr  *hp = NULL;
    char            *bp = NULL;

    /*
     * Read the HTTP header followed by the IPP response header.
     * They can be returned in multiple read attempts.  Use the
     * Content-Length specifier to determine how much to read.
     */
    success = 0;
    bufsz = IOBUFSZ;
    if (NULL == (bp = malloc(IOBUFSZ)))
    {
        log_sys("printer_status: can't allocate read buffer");
    }

    while ((nr = tread(sockfd, bp, IOBUFSZ, 5)) > 0)
    {
        /*
         * Find the status.  Response starts with "HTTP/x.y"
         * so we can skip the first 8 characters.
         跳过HTTP/x.y和报文开始的所有空格，其后应该是数字的状态码，
         如果不是，在日志中记录报文内容
         */
        cp = bp + 8;
        while (isspace((int)*cp))
        {
            cp++;
        }
        statcode = cp;
        while (isdigit((int)*cp))
        {
            cp++;
        }

        /*空格后跟的不是数字*/
        if (cp == statcode)
        {
            /* Bad format; log it and move on */
            log_msg(bp);
        }
        else 
        {
            /*在响应中找到数字状态码，将其开始的非数字字符转换成空字节\0,
            体后应该跟随一个表明出错原因的字符串*/
            *cp++ = '\0';
            reason = cp;
            while (*cp != '\r' && *cp != '\n')
            {
                cp++;
            }
            *cp = '\0';
            code = atoi(statcode);

            /*如果仅是提供信息的报文，忽略并继续循环*/
            if (HTTP_INFO(code))
            {
                continue;
            }

            if (!HTTP_SUCCESS(code)) 
            { 
                /* probable error: log it */
                bp[nr] = '\0';
                log_msg("error: %s", reason);
                break;
            }

            /*
             * The HTTP request was okay, but we still
             * need to check the IPP status.  First
             * search for the Content-Length specifier.
             HTTP请求成功，检查IPP状态
             */
            i = cp - bp;
            for (;;) 
            {
                while (*cp != 'C' && *cp != 'c' && i < nr) 
                {
                    cp++;
                    i++;
                }

                if (i >= nr &&      /* get more header */
                    ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
                {
                    goto out;
                }
                cp = &bp[i];        //重置cp
                if (strncasecmp(cp, "Content-Length:", 15) == 0) 
                {
                    cp += 15;
                    while (isspace((int)*cp))
                    {
                        cp++;
                    }
                    contentlen = cp;
                    while (isdigit((int)*cp))
                    {
                        cp++;
                    }
                    *cp++ = '\0';
                    i = cp - bp;
                    len = atoi(contentlen);
                    break;
                }
                else 
                {
                    cp++;
                    i++;
                }
            }

            if (i >= nr &&    /* get more header */
                ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
            {
                goto out;
            }
            cp = &bp[i];

            found = 0;
            while (!found) 
            {  
                /* look for end of HTTP header */
                while (i < nr - 2) 
                {
                    if (('\n' == *cp) && ('\r' == *(cp + 1))  && \
                       ('\n' == *(cp + 2))) 
                    {
                        found = 1;
                        cp += 3;
                        i += 3;
                        break;
                    }
                    cp++;
                    i++;
                }
                if (i >= nr &&    /* get more header */
                    ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
                {
                    goto out;
                }
                cp = &bp[i];
            }

            if (nr - i < len &&    /* get more header */
                ((nr = readmore(sockfd, &bp, i, &bufsz)) < 0))
            {
                goto out;
            }
            cp = &bp[i];

            hp = (struct ipp_hdr *)cp;
            i = ntohs(hp->Status);
            jobid = ntohl(hp->request_id);

            if (jobid != jp->jobid)
            {
                /*
                 * Different jobs.  Ignore it.
                 */
                log_msg("jobid %ld status code %d", jobid, i);
                break;
            }

            if (STATCLASS_OK(i))
            {
                success = 1;
            }
            break;
        }
    }

out:
    free(bp);

    if (nr < 0)
    {
        log_msg("jobid %ld: error reading printer response: %s", \
            jobid, strerror(errno));
    }

    return(success);
}
