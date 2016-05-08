/*17-28 ����client�������������--����ͻ�����Ϣ*/

#include	"opend.h"

#define NALLOC  10                  /* # client structs to alloc/realloc for */

/*����δ����ռ��client�ṹ����ռ䣬���ѷ���ռ䣬���ٷ���NALLOC��client�Ŀռ䣬����ʼ��*/
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
 ����ͻ��˵�fd&uid�����洢�ռ䲻�㣬��malloc���ٱ���
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
    /*�ػ����̣���Ҫ��¼��־*/
	log_quit("can't find client entry for fd %d", fd);
}
/*��̬�����洢������--�����õ��κ���Ҫ����δ֪�����ݽṹ�ĳ���*/