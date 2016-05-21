/*20-1 ����һ�����ݿⲢ������д3����¼*/

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
.idx�ļ���
0       53      35      0
�����ļ���һ�У���������ָ��(0--��������Ϊ��)������ɢ������ָ��(53,35,0)
ɢ����1:ƫ��53gamma-->ƫ��17alpha
ɢ����2:ƫ��35beta
ɢ����3��ƫ��0����ɢ����

0       10Alpha:0:6
һ��������¼�Ľṹ��
1.����ָ��chain prt--0--��ʾ�˼�¼��ɢ����������һ����10--������¼����idx len--������¼ʣ�ಿ�ֵĳ��ȣ�
����read��ȡchain ptr + idx len
2.ʣ���÷ָ���sep���������ֶΣ���key�����ݼ�¼ƫ����dat off�����ݼ�¼����dat len
����3�ֶβ�����������Ҫ�ָ���--�ָ������ܳ����ڼ��У��ָ����Ǳ��룬�������ı���¼�鿴ֱ��

0       10beta:6:14
17      11gamma:20:8

.dat�ļ���
data1(\n) -- ��һ�����ݼ�¼���������з����ܳ�����6
Data for beta
reccord3

*/