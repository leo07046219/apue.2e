/*17-17 ��UNIX���׽����Ͻ���ƾ֤�������ط����ߵ��û�ID*/

#include "apue.h"
#include <sys/socket.h>		                /* struct msghdr */
#include <sys/un.h>

#if defined(SCM_CREDS)			            /* BSD interface */
#define CREDSTRUCT		cmsgcred
#define CR_UID			cmcred_uid
#define CREDOPT			LOCAL_PEERCRED
#define SCM_CREDTYPE	SCM_CREDS
#elif defined(SCM_CREDENTIALS)	            /* Linux interface */
#define CREDSTRUCT		ucred
#define CR_UID			uid
#define CREDOPT			SO_PASSCRED
#define SCM_CREDTYPE	SCM_CREDENTIALS
#else
#error passing credentials is unsupported!
#endif

/* size of control buffer to send/recv one file descriptor */
#define RIGHTSLEN	CMSG_LEN(sizeof(int))
#define CREDSLEN	CMSG_LEN(sizeof(struct CREDSTRUCT))
#define	CONTROLLEN	(RIGHTSLEN + CREDSLEN)

static struct cmsghdr	*pCmsghdr = NULL;		/* malloc'ed first time */

/*
 * Receive a file descriptor from a server process.  Also, any data
 * received is passed to (*userfunc)(STDERR_FILENO, buf, nbytes).
 * We have a 2-byte protocol for receiving the fd from send_fd().
 */
int recv_ufd(int fd, uid_t *uidptr, \
         ssize_t (*userfunc)(int, const void *, size_t))
{
	struct cmsghdr		*cmp = NULL;
	struct CREDSTRUCT	*credp = NULL;
	int					newfd = 0, nr = 0, status = 0;
	char				*ptr = NULL;
	char				buf[MAXLINE];
	struct iovec		iov[1];
	struct msghdr		msg;
	const int			on = 1;

    memset((char *)iov, 0, sizof(iov));
    memset((char *)buf, 0, sizof(buf));
    memset((char *)&msg, 0, sizof(msg));

	status = -1;
	newfd = -1;

	if (setsockopt(fd, SOL_SOCKET, CREDOPT, &on, sizeof(int)) < 0) 
    {
		err_ret("setsockopt failed");
		return(-1);
	}

	for ( ; ; ) 
    {
		iov[0].iov_base = buf;
		iov[0].iov_len  = sizeof(buf);
		msg.msg_iov     = iov;
		msg.msg_iovlen  = 1;
		msg.msg_name    = NULL;
		msg.msg_namelen = 0;

        if ((NULL == pCmsghdr) \
            && (NULL == (pCmsghdr = malloc(CONTROLLEN))))
        {
            return(-1);
        }

		msg.msg_control    = pCmsghdr;
		msg.msg_controllen = CONTROLLEN;

		if ((nr = recvmsg(fd, &msg, 0)) < 0) 
        {
			err_sys("recvmsg error");
		} 
        else if (0 == nr) 
        {
			err_ret("connection closed by server");
			return(-1);
		}

		/*
		 * See if this is the final data with null & status.  Null
		 * is next to last byte of buffer; status byte is last byte.
		 * Zero status means there is a file descriptor to receive.
		 */
		for (ptr = buf; ptr < &buf[nr]; ) 
        {
			if (0 == *ptr++) 
            {
                if (ptr != &buf[nr - 1])
                {
                    err_dump("message format error");
                }

 				status = *ptr & 0xFF;	/* prevent sign extension */

 				if (0 == status) 
                {
                    if (msg.msg_controllen != CONTROLLEN)
                    {
                        err_dump("status = 0 but no fd");
                    }

					/* process the control data */
					for (cmp = CMSG_FIRSTHDR(&msg); \
					  cmp != NULL; cmp = CMSG_NXTHDR(&msg, cmp))
                    {
                        if (cmp->cmsg_level != SOL_SOCKET)
                        {
                            continue;
                        }

						switch (cmp->cmsg_type) 
                        {
						case SCM_RIGHTS:
							newfd = *(int *)CMSG_DATA(cmp);
							break;
						case SCM_CREDTYPE:
							credp = (struct CREDSTRUCT *)CMSG_DATA(cmp);
                            /*���ط����ߵ��û�ID*/
							*uidptr = credp->CR_UID;
                            break;
                        default:
                            break;
						}
					}
				} 
                else 
                {
					newfd = -status;
				}

				nr -= 2;
			}
		}

        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
        {
            return(-1);
        }

        if (status >= 0)	/* final data has arrived */
        {
            return(newfd);	/* descriptor, or -status */
        }
	}
}
