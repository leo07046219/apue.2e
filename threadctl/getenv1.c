/*12-3 getenv的非可重入版本*/

#include <limits.h>
#include <string.h>
#include "apue.h"

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