/*6-1 getpwnam函数*/
#include <pwd.h>
#include <stddef.h>
#include <string.h>

struct passwd *getpwnam(const char *name)
{
    struct passwd  *ptr = NULL;

    setpwent();/*关闭getpwnam之前可能打开的而未关闭的文件*/

    while ((ptr = getpwent()) != NULL)
    {
        if (strcmp(name, ptr->pw_name) == 0)
        {
            break;		/* found a match */
        }
    }

    endpwent();

    return(ptr);	/* ptr is NULL if no match found */
}
