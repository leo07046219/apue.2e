/*17-18 open.h头文件，用户open1服务器版本*/

#include "apue.h"
#include <errno.h>

#define	CL_OPEN "open"          /* client's request for server */

int csopen(char *, int);
