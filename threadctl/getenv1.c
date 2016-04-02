/*12-3 getenv�ķǿ�����汾*/

#include <limits.h>
#include <string.h>
#include "apue.h"

/*�����е���getenv���̷߳��ص��ַ����������ͬһ��̬�������У�
�಻������*/
static char envbuf[ARG_MAX];

extern char **environ;

char *getenv(const char *name)
{
	int i = 0, len = 0;

	len = strlen(name);

	for (i = 0; environ[i] != NULL; i++) 
    {
		if ((strncmp(name, environ[i], len) == 0) &&
		  (environ[i][len] == '=')) 
        {
			strcpy(envbuf, &environ[i][len+1]);
			return(envbuf);
		}
	}

	return(NULL);
}
/*
1.�̰߳�ȫ��
��ͬһʱ�̿��Ա�����̰߳�ȫ�ص���

2.���������ԭ��
��������Ǳ����ھ�̬�ڴ滺������

3.����ʱ����Ƿ������
֧���̰߳�ȫ������os����<unistd.h>�ж���_POSIX_THREAD_SAFE_FUNCTIONS

4.�����뻯  _r
����������Ϊ�Լ��ṩ��

5.������VS�첽-�źŰ�ȫ
һ�������Զ���߳��ǿ�����ģ������̰߳�ȫ�ģ�����˵�����źŴ������Ҳ�ǿ�����ġ�
�첽-�źŰ�ȫ���������첽�źŴ������������ǰ�ȫ��

*/