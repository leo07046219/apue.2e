/*12-5 �̰߳�ȫ��getenv�ļ��ݰ汾*/

#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include "apue.h"

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
pthread_mutex_t env_mutex = PTHREAD_MUTEX_INITIALIZER;

extern char **environ;

static void thread_init(void)
{
	pthread_key_create(&key, free);
}

char * getenv(const char *name)
{
	int		i, len;
	char	*envbuf;

    /*ȷ��ֻΪ��Ҫʹ�õ��߳�˽�����ݴ���һ����*/
	pthread_once(&init_done, thread_init);

	pthread_mutex_lock(&env_mutex);
    /*���޷��޸�Ӧ�ó�����ֱ��ʹ���½ӿ�ʱ��
    �������߳�˽��������ά��ÿ���̵߳����ݻ������ĸ��������ڴ�Ÿ��Եķ����ַ���*/
	envbuf = (char *)pthread_getspecific(key);
	if (NULL == envbuf) 
    {
        /*�����첽--�źŰ�ȫ*/
		envbuf = malloc(ARG_MAX);
		if (NULL == envbuf) 
        {
			pthread_mutex_unlock(&env_mutex);
			return(NULL);
		}
		pthread_setspecific(key, envbuf);
	}

	len = strlen(name);
	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) &&
		  (environ[i][len] == '=')) 
        {
			strcpy(envbuf, &environ[i][len+1]);
			pthread_mutex_unlock(&env_mutex);
			return(envbuf);
		}
	}
	pthread_mutex_unlock(&env_mutex);

	return(NULL);
}
/*���á��������Զ��ͷ�malloc�������߳�˽�����ݣ������ݱ����ڴ�ŷ���ֵ
�����е�����������������ɺ�ϵͳ�����Ƿ��з�null���߳�˽������ֵ���������
���У��ٴε�������������һֱ�ظ����߳����еļ���Ϊnullֵ�߳�˽�����ݣ���ﵽ����Դ�����*/