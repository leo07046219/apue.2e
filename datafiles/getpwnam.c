/*6-1 getpwnam����*/
#include <pwd.h>
#include <stddef.h>
#include <string.h>

struct passwd *getpwnam(const char *name)
{
    struct passwd  *ptr = NULL;

    setpwent();/*�ر�getpwnam֮ǰ���ܴ򿪵Ķ�δ�رյ��ļ�*/

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
