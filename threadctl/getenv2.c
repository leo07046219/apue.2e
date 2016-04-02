/*12-4 getenv�Ŀ�����(�̰߳�ȫ)�汾*/

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/*�������Լ��ṩ�����������������̵߳ĸ���*/
extern char **environ;

pthread_mutex_t env_mutex;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;

static void thread_init(void)
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
    
    /*��ʹ�õ��Ƿǵݹ黥���������̴߳��źŴ�������е���getenv_rʱ�����п��ܳ�������
    ����źŴ���������߳�ִ��getenv_rʱ�ж��˸��̣߳����������Ѿ�ռ�м�����env_mutex,
    ���������߳���ͼ������������ļ������̾ͻᱻ���������յ����߳̽�������״̬��

    �����ʹ�õݹ黥������ֹ�����̸߳ı䵱ǰ���ڲ鿴�����ݽṹ��

    ͬʱ����Ҫ��ֹ�����źŴ�����������������pthread�������Ǳ�֤�źŰ�ȫ�����Բ��ܰ���������������(?)*/

	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&env_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

int getenv_r(const char *name, char *buf, int buflen)
{
	int i, len, olen;

	pthread_once(&init_done, thread_init);
	len = strlen(name);

    /*��������֤������������ַ���ʱ���������޸ģ��Ի����б�ķ������л�*/
	pthread_mutex_lock(&env_mutex);

	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) && \
		  (environ[i][len] == '=')) 
        {
			olen = strlen(&environ[i][len+1]);
			if (olen >= buflen) 
            {
				pthread_mutex_unlock(&env_mutex);
				return(ENOSPC);
			}
			strcpy(buf, &environ[i][len+1]);
			pthread_mutex_unlock(&env_mutex);
			return(0);
		}
	}

	pthread_mutex_unlock(&env_mutex);

	return(ENOENT);
}