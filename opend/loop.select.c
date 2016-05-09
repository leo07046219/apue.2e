/*17-30 使用select的loop函数*/

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

	/* obtain fd to listen for client requests on 获取侦听的fd*/
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
			/* 接收到新的客户端请求，accept new client request */
            if ((clifd = serv_accept(listenfd, &uid)) < 0)
            {
                log_sys("serv_accept error: %d", clifd);
            }

            /*保存客户端信息*/
			i = client_add(clifd, uid);

            /* 更新select的最大fd和allset，继续select，，max fd for select() */
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
            /* 跳过非法fd， go through client[] array */
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
                    /*没从客户端读取到数据--客户进程已终止，将此客户端删除，并结束对相关fd的select*/
					log_msg("closed: uid %d, fd %d", \
					  client[i].uid, clifd);

					client_del(clifd);      /* client has closed cxn */
					FD_CLR(clifd, &allset);
					close(clifd);
				} 
                else 
                {	
                    /* process client's request 处理客户端请求*/
					request(buf, nread, clifd, client[i].uid);
				}
			}
		}
	}
}
