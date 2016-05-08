/*17-25 cli_args函数*/

#include	"opend.h"

/*
 * This function is called by buf_args(), which is called by
 * request().  buf_args() has broken up the client's buffer
 * into an argv[]-style array, which we now process.
 */
int cli_args(int argc, char **argv)
{
	if (argc != 3 || strcmp(argv[0], CL_OPEN) != 0) 
    {
		strcpy(errmsg, "usage: <pathname> <oflag>\n");
		return(-1);
	}

	pathname = argv[1];		/* save ptr to pathname to open */
	oflag = atoi(argv[2]);

	return(0);
}
/*验证客户进程发送的参数个数是否正确，把路径名和打开模式存放在全局变量中*/