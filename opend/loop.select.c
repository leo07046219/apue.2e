/*17-30 ʹ��select��loop����*/

#include	"opend.h"
#include	<sys/time.h>
#include	<sys/select.h>

void loop(void)
{
	int		i = 0, n = 0, maxfd = 0, maxi = 0, listenfd = 0, clifd = 0, nread = 0;
	char	buf[MAXLINE];
	uid_t	uid;
	fd_set	rset, allset;

    memset(&buf, 0, sizeof(buf));

	FD_ZERO(&allset);

	/* obtain fd to listen for client requests on ��ȡ������fd*/
    if ((listenfd = serv_listen(CS_OPEN)) < 0)
    {
        log_sys("serv_listen error");
    }

	FD_SET(listenfd, &allset);
	maxfd = listenfd;
	maxi = -1;

	for ( ; ; ) 
    {
		rset = allset;                      /* rset gets modified each time around */
        if ((n = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0)
        {
            log_sys("select error");
        }

		if (FD_ISSET(listenfd, &rset)) 
        {
			/* ���յ��µĿͻ�������accept new client request */
            if ((clifd = serv_accept(listenfd, &uid)) < 0)
            {
                log_sys("serv_accept error: %d", clifd);
            }

            /*����ͻ�����Ϣ*/
			i = client_add(clifd, uid);

            /* ����select�����fd��allset������select����max fd for select() */
			FD_SET(clifd, &allset);
            if (clifd > maxfd)
            {
                maxfd = clifd;
            }
            if (i > maxi)
            {
                maxi = i;                   /* max index in client[] array */
            }

			log_msg("new connection: uid %d, fd %d", uid, clifd);
			continue;
		}

		for (i = 0; i <= maxi; i++) 
        {	
            /* �����Ƿ�fd�� go through client[] array */
            if ((clifd = client[i].fd) < 0)
            {
                continue;
            }

			if (FD_ISSET(clifd, &rset)) 
            {
				/* read argument buffer from client */
				if ((nread = read(clifd, buf, MAXLINE)) < 0) 
                {
					log_sys("read error on fd %d", clifd);
				} 
                else if (0 == nread) 
                {
                    /*û�ӿͻ��˶�ȡ������--�ͻ���������ֹ�����˿ͻ���ɾ���������������fd��select*/
					log_msg("closed: uid %d, fd %d", \
					  client[i].uid, clifd);

					client_del(clifd);      /* client has closed cxn */
					FD_CLR(clifd, &allset);
					close(clifd);
				} 
                else 
                {	
                    /* process client's request ����ͻ�������*/
					request(buf, nread, clifd, client[i].uid);
				}
			}
		}
	}
}
