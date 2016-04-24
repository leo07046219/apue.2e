/*16-2 ֧�����Ե�����*/

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
1.��ʾ�˴���˲ʱconnect����ķ������ڸ��غ��صķ������Ͽ��ܷ�����
2.ָ�������㷨��ÿѭ��һ�Σ�����ÿ�γ��Ե��ӳ٣�ֱ������ӳ�128�룻
*/