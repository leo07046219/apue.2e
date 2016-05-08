/*17-28 操纵client数组的三个函数--保存客户端信息*/

#include	"opend.h"

#define NALLOC  10                  /* # client structs to alloc/realloc for */

/*对于未分配空间的client结构分配空间，若已分配空间，则再分配NALLOC个client的空间，并初始化*/
static void client_alloc(void)      /* alloc more entries in the client[] array */
{
	int		i;

    if (NULL == client)
    {
        client = malloc(NALLOC * sizeof(Client));
    }
    else
    {
        client = realloc(client, (client_size + NALLOC)*sizeof(Client));
    }

    if (NULL == client)
    {
        err_sys("can't alloc for client array");
    }

	/* initialize the new entries */
    for (i = client_size; i < client_size + NALLOC; i++)
    {
        client[i].fd = -1;          /* fd of -1 means entry available */
    }

	client_size += NALLOC;
}

/*
 * Called by loop() when connection request from a new client arrives.
 保存客户端的fd&uid，若存储空间不足，则malloc，再保存
 */
int client_add(int fd, uid_t uid)
{
	int i = 0;

    if (NULL == client)             /* first time we're called */
    {
        client_alloc();
    }

    /* find an available entry */
again:
	for (i = 0; i < client_size; i++) 
    {
		if (-1 == client[i].fd) 
        {	
			client[i].fd = fd;
			client[i].uid = uid;
			return(i);              /* return index in client[] array */
		}
	}
	/* client array full, time to realloc for more */
	client_alloc();

	goto again;		                /* and search again (will work this time) */
}

/*
 * Called by loop() when we're done with a client.
 */
void client_del(int fd)
{
	int i = 0;

	for (i = 0; i < client_size; i++) 
    {
		if (client[i].fd == fd) 
        {
			client[i].fd = -1;
			return;
		}
	}
    /*守护进程，需要记录日志*/
	log_quit("can't find client entry for fd %d", fd);
}
/*动态伸缩存储区机制--可以用到任何需要保存未知个数据结构的场景*/