/*20-1 建立一个数据库并向其中写3条记录*/

#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>

int main(void)
{
	DBHANDLE	db = NULL;

    if (NULL == (db = db_open("db4", O_RDWR | O_CREAT | O_TRUNC, \
        FILE_MODE)))
    {
        err_sys("db_open error");
    }

    if (db_store(db, "Alpha", "data1", DB_INSERT) != 0)
    {
        err_quit("db_store error for alpha");
    }

    if (db_store(db, "beta", "Data for beta", DB_INSERT) != 0)
    {
        err_quit("db_store error for beta");
    }

    if (db_store(db, "gamma", "record3", DB_INSERT) != 0)
    {
        err_quit("db_store error for gamma");
    }

	db_close(db);

	exit(0);
}
/*
.idx文件：
0       53      35      0
0       10Alpha:0:6
0       10beta:6:14
17      11gamma:20:8
.dat文件：
data1
Data for beta
reccord3

*/