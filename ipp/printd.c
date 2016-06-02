/*21-5 ��ӡ���ѻ��ػ����̳���*/

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
  http����״̬
  */
#define HTTP_INFO(x)    ((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(x) ((x) >= 200 && (x) <= 299)

  /*���ѻ��ػ�����ʹ��job��worker_thread�ṹ��������Ӧ��ӡ��ҵ�ͽ��մ�ӡ������߳�*/

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
struct addrinfo        *printer;//��ӡ�������ַ
char                    *printer_name;//��ӡ����������
pthread_mutex_t        configlock = PTHREAD_MUTEX_INITIALIZER;//������reread����
int                    reread;//��ʾ�ػ�������Ҫ�ٴζ�ȡ�����ļ�

/*
 * Thread-related stuff.�߳�˫������ͷ�����ڽ������Կͻ��˵��ļ�
 */
struct worker_thread    *workers;
pthread_mutex_t        workerlock = PTHREAD_MUTEX_INITIALIZER;
sigset_t                mask;

/*
 * Job-related stuff.������ҵ����
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
 *main���������飺��ʼ���ػ�����+�������Կͻ��˵�����
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

    /*����SIGPIPE�ź�
    ��Ҫдsocket fd�����Ҳ�����д�������SIGPIPE����ΪĬ�϶�����ɱ������*/
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        log_sys("sigaction failed");
    }

    /*�����̵߳��ź����룬�����������߳̾��̳�����ź�����*/
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);   //�����ػ������ٴζ�ȡ�����ļ�
    sigaddset(&mask, SIGTERM);  //�����ػ�������ִ���������������˳�
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        log_sys("pthread_sigmask failed");
    }

    /*��ʼ����ҵ����ȷ��ֻ��һ���ػ����̵ĸ���������*/
    init_request();

    /*��ʼ����ӡ����Ϣ*/
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

    /*��ʼ�������� �� ��select��fd*/
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

    /*����Ȩ�ޣ���С��Ȩԭ���Ա������ػ����̳����н�ϵͳ��¶���κο��ܵĹ���*/
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

    /*���������źŵ��߳�*/
    pthread_create(&tid, NULL, printer_thread, NULL);

    /*�����ڴ�ӡ��ͨ�ŵ��߳�*/
    pthread_create(&tid, NULL, signal_thread, NULL);

    /*��/var/spool/printerĿ¼�������κι������ҵ*/
    build_qonstart();

    /*����ػ����̵����г�ʼ��������������־*/
    log_msg("daemon initialized");

    for (;;)
    {
        /*����ֱ��ʹ��rendezvous��select���޸���ڲ���*/
        rset = rendezvous;
        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0)
        {
            log_sys("select failed");
        }

        /*���յ�һ���������󣬾�accept֮�����������̴߳���ͻ�������*/
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

    /*�����ļ�����֤�ػ����̵�Ψһ��*/
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

    /*���ļ��մ��������Ϊ�գ���ô��nextjob����Ϊ1*/
    if (0 == n)
    {
        nextjob = 1;
    }
    else
    {
        nextjob = atol(name);
    }
    /*���ܹرո��ļ�����Ϊ�⽫�ͷ��Ѿ������������д��*/
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

    /*����ӡ���̷߳��źţ����߸��߳���һ����ҵ������*/
    pthread_cond_signal(&jobwait);
}

/*
 * Replace a job back on the head of the list.
 *����ҵ���뵽������ҵ�б�ͷ��
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
 *�Ӵ洢��/var/spool/printer/reqs�еĴ����ļ�����һ���ڴ��еĴ�ӡ��ҵ�б�
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
    /*����Ŀ¼�ӳ�.��..֮���������Ŀ����ȡ�������ļ��е�printreq�ṹ*/
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
        /*���ļ���ת��Ϊ��ҵid*/
        jobid = atol(entp->d_name);
        log_msg("adding job %ld to queue", jobid);
        add_job(&req, jobid);
    }

    closedir(dirp);
}

/*
 * Accept a print job from a client.
 *�ӿͻ���print�����н���Ҫ��ӡ���ļ�
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

    /*��װ�߳����������*/
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
        /*�������ݲ��ԣ���ͻ��˷��ش���ţ����˳��߳� --- ������ȡ����,����ط��õ�*/
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
        /*����Ҫ��ʾ�ر�sockfd��������pthread_exitʱ���߳����������ᴦ���ļ�������*/

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
     * Read the file and store it in the spool directory.���������ļ�
     */
    first = 1;
    while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0)
    {
        if (first)
        {
            first = 0;
            if (strncmp(buf, "%!PS", 4) != 0)   //PostScript����%!PSģʽ��ͷ��
            {
                req.flags |= PR_TEXT;   //���ı��ļ�
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
            /*����Ҫ��ʾ�ر�sockfd��������pthread_exitʱ���߳����������ᴦ���ļ�������*/
            unlink(name);

            pthread_exit((void *)1);
        }
    }
    close(fd);

    /*
     * Create the control file.��ס��ӡ��������ҵidΪ�ļ���
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
    /*�رտ����ļ�������
    �ͽ��̲�ͬ����һ���߳̽������ҽ������������߳�ʱ���ļ������������Զ��رգ�
    ������رղ���Ҫ���ļ����������ս��ľ���Դ*/
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
*��һ��worker_thread�ṹ���뵽��߳��б���
* LOCKING: acquires and releases workerlock.
*/
void add_worker(pthread_t tid, int sockfd)
{
    struct worker_thread    *wtp = NULL;

    /*client_cleanup()��free*/
    if ((wtp = malloc(sizeof(struct worker_thread))) == NULL)
    {
        log_ret("add_worker: can't malloc");
        pthread_exit((void *)1);
    }
    wtp->tid = tid;
    wtp->sockfd = sockfd;

    /*���ṹ�ӵ��б�ͷ��*/
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
        /*ʵ�ʵ�ɾ��������ÿ���̵߳���ʵ�ʵ�ɾ����ʱ���� --- �̰߳�ȫ�˳�����*/
        pthread_cancel(wtp->tid);
    }

    pthread_mutex_unlock(&workerlock);
}

/*
 * Cancellation routine for the worker thread.
 *��ͻ�������ͨ�ŵĹ������̵߳��߳����������
 �÷����������pthread_cleanup_pop��������Ӧһ��ɾ������ʱ���ú��������á�
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

    /*�ͷ�֮ǰ������ڴ�*/
    if (wtp != NULL)
    {
        close(wtp->sockfd);
        free(wtp);
    }
}

/*
 * Deal with signals.
 *�ɸ����źŴ�����߳�����
 * LOCKING: acquires and releases configlock.
 */
void *signal_thread(void *arg)
{
    int        err = 0, signo = 0;

    for (;;)
    {
        /*�ȴ��ź��е�һ������*/
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
            /*��ӡ�ػ����̳���printer_thread�и���reread��־λ�����Ƿ��ض������ļ�*/
            pthread_mutex_unlock(&configlock);
            break;

        case SIGTERM:
            /*ɱ�����й����߳�*/
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
 *��������ӡ����IPP�ײ����ѡ��
 * LOCKING: none.
 */
char * add_option(char *cp, int tag, char *optname, char *optval)
{
    int        n = 0;

    /*һЩ�������ܹ�������SPARC�������ܴ������ַװ��һ��������
    ͨ�����淽ʽʵ�֣�
    nתת�������ֽ��򣬸���u.s,��u.c[2]����cp*/
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

    /*�����ײ�����һ����Ӧ�ÿ�ʼ�ĵ�ַ*/
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

        /*����һ����ҵ��д�뵽/var/spool/printer/jobno��*/
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

        /*�������ӡ����tcp����*/
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
        /*�����ӳ٣�try again*/
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
 *��ȡ���Դ�ӡ���Ĳ�����Ӧ��Ϣ
 * LOCKING: none.
 */
ssize_t readmore(int sockfd, char **bpp, int off, int *bszp)
{
    ssize_t     nr;
    char        *bp = *bpp;
    int         bsz = *bszp;

    if (off >= bsz)
    {
        /*��̬����������*/
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
 *��ȡ��ӡ����һ����ӡ��ҵ������ĵ���Ӧ��Ϣ
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
         ����HTTP/x.y�ͱ��Ŀ�ʼ�����пո����Ӧ�������ֵ�״̬�룬
         ������ǣ�����־�м�¼��������
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

        /*�ո����Ĳ�������*/
        if (cp == statcode)
        {
            /* Bad format; log it and move on */
            log_msg(bp);
        }
        else 
        {
            /*����Ӧ���ҵ�����״̬�룬���俪ʼ�ķ������ַ�ת���ɿ��ֽ�\0,
            ���Ӧ�ø���һ����������ԭ����ַ���*/
            *cp++ = '\0';
            reason = cp;
            while (*cp != '\r' && *cp != '\n')
            {
                cp++;
            }
            *cp = '\0';
            code = atoi(statcode);

            /*��������ṩ��Ϣ�ı��ģ����Բ�����ѭ��*/
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
             HTTP����ɹ������IPP״̬
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
                cp = &bp[i];        //����cp
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
