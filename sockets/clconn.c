/*16-2 支持重试的连接*/

#include "apue.h"
#include <sys/socket.h>

#define MAXSLEEP 128

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
	int nsec;

	/*
	 * Try to connect with exponential backoff.
	 */
	for (nsec = 1; nsec <= MAXSLEEP; nsec <<= 1) 
    {
		if (connect(sockfd, addr, alen) == 0) 
        {
			/*
			 * Connection accepted.
			 */
			return(0);
		}

		/*
		 * Delay before trying again.
		 */
        if (nsec <= MAXSLEEP / 2)
        {
            sleep(nsec);
        }
	}

	return(-1);
}
/*
1.演示了处理瞬时connect错误的方法，在负载很重的服务器上可能发生；
2.指数补偿算法，每循环一次，增加每次尝试的延迟，直到最大延迟128秒；
*/