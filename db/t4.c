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
索引文件第一行：空闲链表指针(0--空闲链表为空)，三个散列链的指针(53,35,0)
散列链1:偏移53gamma-->偏移17alpha
散列链2:偏移35beta
散列链3：偏移0，空散列链

0       10Alpha:0:6
一条索引记录的结构：
1.链表指针chain prt--0--表示此记录是散列链表的最后一条，10--索引记录长度idx len--索引记录剩余部分的长度，
两个read读取chain ptr + idx len
2.剩下用分隔符sep隔开的三字段：键key，数据记录偏移量dat off，数据记录长度dat len
上述3字段不定长，∴需要分隔符--分隔符不能出现在键中，分隔符非必须，仅用于文本记录查看直观

0       10beta:6:14
17      11gamma:20:8

.dat文件：
data1(\n) -- 第一条数据记录，包含换行符的总长度是6
Data for beta
reccord3

*/