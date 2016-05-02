/*17-11 send_err����*/

#include "apue.h"

/*
 * Used when we had planned to send an fd using send_fd(),
 * but encountered an error instead.  We send the error back
 * using the send_fd()/recv_fd() protocol.
 */
int send_err(int fd, int errcode, const char *pMsg)
{
	int     n = 0;

    assert(pMsg != NULL);

    if ((n = strlen(pMsg)) > 0)
    {
        if (writen(fd, pMsg, n) != n)    /* send the error message */
        {
            return(-1);
        }
    }

    if (errcode >= 0)
    {
        errcode = -1;                   /* must be negative */
    }

    if (send_fd(fd, errcode) < 0)
    {
        return(-1);
    }

	return(0);
}
/*
�ڷ���fd֮ǰ�ȷ���mgs��Ϣ
*/