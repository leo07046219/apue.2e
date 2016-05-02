/*17-8 UNIXÓòÌ×½Ó×ÖµÄserv_listenº¯Êı*/

#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define QLEN	10

/*
 * Create a server endpoint of a connection.
 * Returns fd if all OK, <0 on error.
 */
int serv_listen(const char *pName)
{
	int                 fd = 0, len = 0, err = 0, rval = 0;
	struct sockaddr_un  un;

    assert(pName != NULL);

    memset((char *)&un, 0, sizeof(un));

	/* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        return(-1);
    }

	unlink(pName);	                    /* in case it already exists */

	/* fill in socket address structure */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, pName);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(pName);

	/* bind the pName to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0) 
    {
		rval = -2;
		goto errExit;
	}

    /* tell kernel we're a server */
	if (listen(fd, QLEN) < 0) 
    {	      
		rval = -3;
		goto errExit;
	}

	return(fd);

errExit:
	err = errno;
	close(fd);
	errno = err;
	return(rval);
}
