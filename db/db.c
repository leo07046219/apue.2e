/*20-3 数据库函数库源代码：包括了多进程并发控制需要的对记录加锁的功能*/

#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>		/* open & db_open flags */
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>	/* struct iovec */

/*
 * Internal index file constants.
 * These are used to construct records in the
 * index file and data file.
 */
#define IDXLEN_SZ	   4	/* index record length (ASCII chars) */
#define SEP         ':'	/* separator char in index record */
#define SPACE       ' '	/* space character */
#define NEWLINE     '\n'	/* newline character */

/*
 * The following definitions are for hash chains and free
 * list chain in the index file.
 */
#define PTR_SZ        6	/* size of ptr field in hash chain */
#define PTR_MAX  999999	/* max file offset = 10**PTR_SZ - 1 */
#define NHASH_DEF	 137	/* default hash table size */
#define FREE_OFF      0	/* free list offset in index file */
#define HASH_OFF PTR_SZ	/* hash table offset in index file */

typedef unsigned long	DBHASH;	/* hash values */
typedef unsigned long	COUNT;	/* unsigned counter */

/*
 * Library's private representation of the database.
 记录一个打开数据库的所有信息
 */
typedef struct 
{
  int    idxfd;  /* fd for index file */
  int    datfd;  /* fd for data file */
  char  *idxbuf; /* malloc'ed buffer for index record */
  char  *datbuf; /* malloc'ed buffer for data record*/
  char  *name;   /* name db was opened under */
  off_t  idxoff; /* offset in index file of index record */
			      /* key is at (idxoff + PTR_SZ + IDXLEN_SZ) */
  size_t idxlen; /* length of index record */
			      /* excludes IDXLEN_SZ bytes at front of record */
			      /* includes newline at end of index record */
  off_t  datoff; /* offset in data file of data record */
  size_t datlen; /* length of data record */
			      /* includes newline at end */
  off_t  ptrval; /* contents of chain ptr in index record */
  off_t  ptroff; /* chain ptr offset pointing to this idx record */
  off_t  chainoff; /* offset of hash chain for this index record */
  off_t  hashoff;  /* offset in index file of hash table */
  DBHASH nhash;    /* current hash table size */

  /*操作成功与否计数--用于分析数据库性能*/
  COUNT  cnt_delok;    /* delete OK */
  COUNT  cnt_delerr;   /* delete error */
  COUNT  cnt_fetchok;  /* fetch OK */
  COUNT  cnt_fetcherr; /* fetch error */
  COUNT  cnt_nextrec;  /* nextrec */
  COUNT  cnt_stor1;    /* store: DB_INSERT, no empty, appended */
  COUNT  cnt_stor2;    /* store: DB_INSERT, found empty, reused */
  COUNT  cnt_stor3;    /* store: DB_REPLACE, diff len, appended */
  COUNT  cnt_stor4;    /* store: DB_REPLACE, same len, overwrote */
  COUNT  cnt_storerr;  /* store error */
} DB;

/*
 * Internal functions.
 私有函数，"_db"打头
 */
static DB     *_db_alloc(int);
static void    _db_dodelete(DB *);
static int	    _db_find_and_lock(DB *, const char *, int);
static int     _db_findfree(DB *, int, int);
static void    _db_free(DB *);
static DBHASH  _db_hash(DB *, const char *);
static char   *_db_readdat(DB *);
static off_t   _db_readidx(DB *, off_t);
static off_t   _db_readptr(DB *, off_t);
static void    _db_writedat(DB *, const char *, off_t, int);
static void    _db_writeidx(DB *, const char *, off_t, int, off_t);
static void    _db_writeptr(DB *, off_t, off_t);

/*
 * Open or create a database.  Same arguments as open(2).
 */
DBHANDLE db_open(const char *pathname, int oflag, ...)
{
	DB			*db = NULL;
	int			len = 0, mode = 0;
	size_t		i = 0;
	char		asciiptr[PTR_SZ + 1],
				hash[(NHASH_DEF + 1) * PTR_SZ + 2];
					/* +2 for newline and null */
	struct stat	statbuff;

	/*
	 * Allocate a DB structure, and the buffers it needs.
	 */
	len = strlen(pathname);
    if (NULL == (db = _db_alloc(len)))
    {
        err_dump("db_open: _db_alloc error for DB");
    }

	db->nhash   = NHASH_DEF;/* hash table size */
	db->hashoff = HASH_OFF;	/* offset in index file of hash table */
    /*TODO: name内存在哪里创建的？*/
	strcpy(db->name, pathname);
	strcat(db->name, ".idx");

	if (oflag & O_CREAT) 
    {
        /*可变参数机制，如果是创建，则会传入mode，读取之*/
		va_list ap;

		va_start(ap, oflag);
		mode = va_arg(ap, int);
		va_end(ap);

		/*
		 * Open index file and data file.
		 */
		db->idxfd = open(db->name, oflag, mode);
		strcpy(db->name + len, ".dat");
		db->datfd = open(db->name, oflag, mode);
	} 
    else 
    {
		/*
		 * Open index file and data file.
		 */
		db->idxfd = open(db->name, oflag);
		strcpy(db->name + len, ".dat");
		db->datfd = open(db->name, oflag);
	}

    /*打开、创建数据库出错--清除db并返回*/
	if (db->idxfd < 0 || db->datfd < 0) 
    {
		_db_free(db);
		return(NULL);
	}

    /*创建数据库，必须加锁*/
	if ((oflag & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) 
    {
		/*
		 * If the database was created, we have to initialize
		 * it.  Write lock the entire file so that we can stat
		 * it, check its size, and initialize it, atomically.
         操作索引文件前先加锁
		 */
        if (writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
        {
            err_dump("db_open: writew_lock error");
        }

        if (fstat(db->idxfd, &statbuff) < 0)
        {
            err_sys("db_open: fstat error");
        }

        /*若索引文件长度为0，则该文件刚刚被创建，
        需要初始化它所包含的空闲链表和散列链表指针*/
		if (0 == statbuff.st_size) 
        {
			/*
			 * We have to build a list of (NHASH_DEF + 1) chain
			 * ptrs with a value of 0.  The +1 is for the free
			 * list pointer that precedes the hash table.
			 */
            /*%*d 将数据库指针从整形转化为字符串*/
			sprintf(asciiptr, "%*d", PTR_SZ, 0);
			hash[0] = 0;
            for (i = 0; i < NHASH_DEF + 1; i++)
            {
                strcat(hash, asciiptr);
            }
            /*构造散列表，并写入索引文件*/
			strcat(hash, "\n");
			i = strlen(hash);
            if (write(db->idxfd, hash, i) != i)
            {
                err_dump("db_open: index file init write error");
            }
		}

        if (un_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
        {
            err_dump("db_open: un_lock error");
        }
	}

    /*清除数据库文件指针*/
	db_rewind(db);

	return(db);
}

/*
 * Allocate & initialize a DB structure and its buffers.
 */
static DB *_db_alloc(int namelen)
{
	DB		*db = NULL;

	/*
	 * Use calloc, to initialize the structure to zero.
	 */
    if (NULL == (db = calloc(1, sizeof(DB))))
    {
        err_dump("_db_alloc: calloc error for DB");
    }

	db->idxfd = db->datfd = -1;				/* descriptors */

	/*
	 * Allocate room for the name.
	 * +5 for ".idx" or ".dat" plus null at end.
	 */
    if (NULL == (db->name = malloc(namelen + 5)))
    {
        err_dump("_db_alloc: malloc error for name");
    }

	/*
	 * Allocate an index buffer and a data buffer.
	 * +2 for newline and null at end.
	 */
    if (NULL == (db->idxbuf = malloc(IDXLEN_MAX + 2)))
    {
        err_dump("_db_alloc: malloc error for index buffer");
    }
    if (NULL == (db->datbuf = malloc(DATLEN_MAX + 2)))
    {
        err_dump("_db_alloc: malloc error for data buffer");
    }
	return(db);
}

/*
 * Relinquish access to the database.
 */
void db_close(DBHANDLE h)
{
	_db_free((DB *)h);	/* closes fds, free buffers & struct */
}

/*
 * Free up a DB structure, and all the malloc'ed buffers it
 * may point to.  Also close the file descriptors if still open.
 */
static void _db_free(DB *pDB)
{
    assert(pDB != NULL);

    if (pDB->idxfd >= 0)
    {
        close(pDB->idxfd);
    }
    if (pDB->datfd >= 0)
    {
        close(pDB->datfd);
    }
    if (pDB->idxbuf != NULL)
    {
        free(pDB->idxbuf);
    }
    if (pDB->datbuf != NULL)
    {
        free(pDB->datbuf);
    }
    if (pDB->name != NULL)
    {
        free(pDB->name);
    }

	free(pDB);
}

/*
 * Fetch a record.  Return a pointer to the null-terminated data.
 */
char *db_fetch(DBHANDLE h, const char *key)
{
	DB      *db = h;
	char	*ptr = NULL;

	if (_db_find_and_lock(db, key, 0) < 0) 
    {
		ptr = NULL;				/* error, record not found */
		db->cnt_fetcherr++;
	} 
    else 
    {
		ptr = _db_readdat(db);	/* return pointer to data */
		db->cnt_fetchok++;
	}

	/*
	 * Unlock the hash chain that _db_find_and_lock locked.
	 */
    if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
    {
        err_dump("db_fetch: un_lock error");
    }

	return(ptr);
}

/*
 * Find the specified record.  Called by db_delete, db_fetch,
 * and db_store.  Returns with the hash chain locked.
 按给定的键查找记录
 */
static int _db_find_and_lock(DB *db, const char *key, int writelock)
{
	off_t	offset, nextoffset;

	/*
	 * Calculate the hash value for this key, then calculate the
	 * byte offset of corresponding chain ptr in hash table.
	 * This is where our search starts.  First we calculate the
	 * offset in the hash table for this key.
     将键变换为散列值，用其计算在文件中相应散列链的起始地址
	 */
	db->chainoff = (_db_hash(db, key) * PTR_SZ) + db->hashoff;
	db->ptroff = db->chainoff;

	/*
	 * We lock the hash chain here.  The caller must unlock it
	 * when done.  Note we lock and unlock only the first byte.
     只锁该散列链开始处的第一个字节，允许多个进程同时搜索不同散列链
	 */
	if (writelock) 
    {
        if (writew_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
        {
            err_dump("_db_find_and_lock: writew_lock error");
        }
	} 
    else 
    {
        if (readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
        {
            err_dump("_db_find_and_lock: readw_lock error");
        }
	}

	/*
	 * Get the offset in the index file of first record
	 * on the hash chain (can be 0).
     从散列链中的第一个指针开始，循环遍历该散列链，查找相同键值
	 */
	offset = _db_readptr(db, db->ptroff);
	while (offset != 0) 
    {
		nextoffset = _db_readidx(db, offset);
        if (strcmp(db->idxbuf, key) == 0)
        {
            break;       /* found a match */
        }
		db->ptroff = offset; /* offset of this (unequal) record */
		offset = nextoffset; /* next one to compare */
	}
	/*
	 * offset == 0 on error (record not found).
	 */
	return(offset == 0 ? -1 : 0);
}

/*
 * Calculate the hash value for a key.
 1.哈希算法：将键中每个ASCII字符乘以这个字符在字符串中以1开始的索引号，求和，对散列表记录项数目取余
 2.散列表项数位137，素数，素数散列通常提供良好的分布特性
 */
static DBHASH _db_hash(DB *db, const char *key)
{
	DBHASH		hval = 0;
	char		c = 0;
	int			i = 0;

    for (i = 1; (c = *key++) != 0; i++)
    {
        hval += c * i;		/* ascii char times its 1-based index */
    }

	return(hval % db->nhash);
}

/*
 * Read a chain ptr field from anywhere in the index file:
 * the free list pointer, a hash table chain ptr, or an
 * index record chain ptr.
 未加锁，调用者需要执行加锁保护
 */
static off_t _db_readptr(DB *db, off_t offset)
{
	char	asciiptr[PTR_SZ + 1];

    memset(asciiptr, 0, sizeof(asciiptr));

    if (lseek(db->idxfd, offset, SEEK_SET) == -1)
    {
        err_dump("_db_readptr: lseek error to ptr field");
    }

    if (read(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
    {
        err_dump("_db_readptr: read error of ptr field");
    }

	asciiptr[PTR_SZ] = 0;		/* null terminate */
	return(atol(asciiptr));
}

/*
 * Read the next index record.  We start at the specified offset
 * in the index file.  We read the index record into db->idxbuf
 * and replace the separators with null bytes.  If all is OK we
 * set db->datoff and db->datlen to the offset and length of the
 * corresponding data record in the data file.
 从索引文件的指定偏移量处读取索引记录
 */
static off_t _db_readidx(DB *db, off_t offset)
{
	ssize_t				i = 0;
	char			*ptr1 = NULL, *ptr2 = NULL;
	char			asciiptr[PTR_SZ + 1], asciilen[IDXLEN_SZ + 1];
	struct iovec	iov[2];

    memset(asciiptr, 0, sizeof(asciiptr));
    memset(asciilen, 0, sizeof(asciilen));
    memset(iov, 0, sizeof(iov));

	/*
	 * Position index file and record the offset.  db_nextrec
	 * calls us with offset==0, meaning read from current offset.
	 * We still need to call lseek to record the current offset.
	 */
    if ((db->idxoff = lseek(db->idxfd, offset, \
        offset == 0 ? SEEK_CUR : SEEK_SET)) == -1)
    {
        err_dump("_db_readidx: lseek error");
    }

	/*
	 * Read the ascii chain ptr and the ascii length at
	 * the front of the index record.  This tells us the
	 * remaining size of the index record.
	 */
	iov[0].iov_base = asciiptr;
	iov[0].iov_len  = PTR_SZ;
	iov[1].iov_base = asciilen;
	iov[1].iov_len  = IDXLEN_SZ;
	if ((i = readv(db->idxfd, &iov[0], 2)) != PTR_SZ + IDXLEN_SZ) 
    {
        if ((0 == i) && (0 == offset))
        {
            return(-1);		/* EOF for db_nextrec */
        }

		err_dump("_db_readidx: readv error of index record");
	}

	/*
	 * This is our return value; always >= 0.
	 */
	asciiptr[PTR_SZ] = 0;        /* null terminate */
	db->ptrval = atol(asciiptr); /* offset of next key in chain */

	asciilen[IDXLEN_SZ] = 0;     /* null terminate */
    if ((db->idxlen = atoi(asciilen)) < IDXLEN_MIN || \
        db->idxlen > IDXLEN_MAX)
    {
        err_dump("_db_readidx: invalid length");
    }

	/*
	 * Now read the actual index record.  We read it into the key
	 * buffer that we malloced when we opened the database.
	 */
    if ((i = read(db->idxfd, db->idxbuf, db->idxlen)) != db->idxlen)
    {
        err_dump("_db_readidx: read error of index record");
    }
    if (db->idxbuf[db->idxlen - 1] != NEWLINE)	/* sanity check */
    {
        err_dump("_db_readidx: missing newline");
    }
	db->idxbuf[db->idxlen-1] = 0;	 /* replace newline with null */

	/*
	 * Find the separators in the index record.
	 */
    if (NULL == (ptr1 = strchr(db->idxbuf, SEP)))
    {
        err_dump("_db_readidx: missing first separator");
    }
	*ptr1++ = 0;				/* replace SEP with null */

    if (NULL == (ptr2 = strchr(ptr1, SEP)))
    {
        err_dump("_db_readidx: missing second separator");
    }
	*ptr2++ = 0;				/* replace SEP with null */

    if (strchr(ptr2, SEP) != NULL)
    {
        err_dump("_db_readidx: too many separators");
    }

	/*
	 * Get the starting offset and length of the data record.
	 */
    if ((db->datoff = atol(ptr1)) < 0)
    {
        err_dump("_db_readidx: starting offset < 0");
    }
    if ((db->datlen = atol(ptr2)) <= 0 || db->datlen > DATLEN_MAX)
    {
        err_dump("_db_readidx: invalid length");
    }
	return(db->ptrval);		/* return offset of next key in chain */
}

/*
 * Read the current data record into the data buffer.
 * Return a pointer to the null-terminated data buffer.
 */
static char *_db_readdat(DB *db)
{
    if (lseek(db->datfd, db->datoff, SEEK_SET) == -1)
    {
        err_dump("_db_readdat: lseek error");
    }

    if (read(db->datfd, db->datbuf, db->datlen) != db->datlen)
    {
        err_dump("_db_readdat: read error");
    }

    if (db->datbuf[db->datlen - 1] != NEWLINE)	/* sanity check */
    {
        err_dump("_db_readdat: missing newline");
    }

	db->datbuf[db->datlen-1] = 0; /* replace newline with null */

	return(db->datbuf);		/* return pointer to data record */
}

/*
 * Delete the specified record.
 */
int db_delete(DBHANDLE h, const char *key)
{
	DB		*db = h;
	int		rc = 0;			/* assume record will be found */

	if (_db_find_and_lock(db, key, 1) == 0) 
    {
		_db_dodelete(db);
		db->cnt_delok++;
	} 
    else 
    {
		rc = -1;			/* not found */
		db->cnt_delerr++;
	}
    if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
    {
        err_dump("db_delete: un_lock error");
    }

	return(rc);
}

/*
 * Delete the current record specified by the DB structure.
 * This function is called by db_delete and db_store, after
 * the record has been located by _db_find_and_lock.
 主要是更写出你两个链表：空闲链表以及与键对应的散列链
 */
static void _db_dodelete(DB *db)
{
	int		i = 0;
	char	*ptr = NULL;
	off_t	freeptr, saveptr;

    assert(db != NULL);
	/*
	 * Set data buffer and key to all blanks.
	 */
    for (ptr = db->datbuf, i = 0; i < db->datlen - 1; i++)
    {
        *ptr++ = SPACE;
    }

	*ptr = 0;	/* null terminate for _db_writedat */
	ptr = db->idxbuf;
    while (*ptr)
    {
        *ptr++ = SPACE;
    }

	/*
	 * We have to lock the free list.
	 */
    if (writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("_db_dodelete: writew_lock error");
    }

	/*
	 * Write the data record with all blanks.
	 */
	_db_writedat(db, db->datbuf, db->datoff, SEEK_SET);

	/*
	 * Read the free list pointer.  Its value becomes the
	 * chain ptr field of the deleted index record.  This means
	 * the deleted record becomes the head of the free list.
	 */
	freeptr = _db_readptr(db, FREE_OFF);

	/*
	 * Save the contents of index record chain ptr,
	 * before it's rewritten by _db_writeidx.
	 */
	saveptr = db->ptrval;

	/*
	 * Rewrite the index record.  This also rewrites the length
	 * of the index record, the data offset, and the data length,
	 * none of which has changed, but that's OK.
	 */
	_db_writeidx(db, db->idxbuf, db->idxoff, SEEK_SET, freeptr);

	/*
	 * Write the new free list pointer.
	 */
	_db_writeptr(db, FREE_OFF, db->idxoff);

	/*
	 * Rewrite the chain ptr that pointed to this record being
	 * deleted.  Recall that _db_find_and_lock sets db->ptroff to
	 * point to this chain ptr.  We set this chain ptr to the
	 * contents of the deleted record's chain ptr, saveptr.
	 */
	_db_writeptr(db, db->ptroff, saveptr);

    if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("_db_dodelete: un_lock error");
    }
}

/*
 * Write a data record.  Called by _db_dodelete (to write
 * the record with blanks) and db_store.
 */
static void _db_writedat(DB *db, const char *data, off_t offset, int whence)
{
	struct iovec	iov[2];
	static char		newline = NEWLINE;

    memset(iov, 0, sizeof(iov));
	/*
	 * If we're appending, we have to lock before doing the lseek
	 * and write to make the two an atomic operation.  If we're
	 * overwriting an existing record, we don't have to lock.
     追加--需加锁，覆盖--无需加锁
	 */
    if (SEEK_END == whence) /* we're appending, lock entire file */
    {
        if (writew_lock(db->datfd, 0, SEEK_SET, 0) < 0)
        {
            err_dump("_db_writedat: writew_lock error");
        }
    }

    if (-1 == (db->datoff = lseek(db->datfd, offset, whence)))
    {
        err_dump("_db_writedat: lseek error");
    }
    /*利用writev实现的写文件同时写长度的机制*/
	db->datlen = strlen(data) + 1;	/* datlen includes newline */

	iov[0].iov_base = (char *) data;
	iov[0].iov_len  = db->datlen - 1;
	iov[1].iov_base = &newline;
	iov[1].iov_len  = 1;
    if (writev(db->datfd, &iov[0], 2) != db->datlen)
    {
        err_dump("_db_writedat: writev error of data record");
    }

    if (SEEK_END == whence)
    {
        if (un_lock(db->datfd, 0, SEEK_SET, 0) < 0)
        {
            err_dump("_db_writedat: un_lock error");
        }
    }

}

/*
 * Write an index record.  _db_writedat is called before
 * this function to set the datoff and datlen fields in the
 * DB structure, which we need to write the index record.
 */
static void _db_writeidx(DB *db, const char *key,
             off_t offset, int whence, off_t ptrval)
{
	struct iovec	iov[2];
	char			asciiptrlen[PTR_SZ + IDXLEN_SZ +1];
	int				len = 0;
	char			*fmt = NULL;

    memset(iov, 0, sizeof(iov));
    memset(asciiptrlen, 0, sizeof(asciiptrlen));

    /*验证散列链中下一个指针有效性*/
    if (((db->ptrval = ptrval) < 0) || (ptrval > PTR_MAX))
    {
        err_quit("_db_writeidx: invalid ptr: %d", ptrval);
    }
    /*基于off_t数据类型，选择传送给sprintf的格式字符串，
    即使32位系统也能提供64位文件偏移量*/
    if (sizeof(off_t) == sizeof(long long))
    {
        fmt = "%s%c%lld%c%d\n";
    }
    else
    {
        fmt = "%s%c%ld%c%d\n";
    }
	sprintf(db->idxbuf, fmt, key, SEP, db->datoff, SEP, db->datlen);
    if ((len = strlen(db->idxbuf)) < IDXLEN_MIN || len > IDXLEN_MAX)
    {
        err_dump("_db_writeidx: invalid length");
    }
	sprintf(asciiptrlen, "%*ld%*d", PTR_SZ, ptrval, IDXLEN_SZ, len);

	/*
	 * If we're appending, we have to lock before doing the lseek
	 * and write to make the two an atomic operation.  If we're
	 * overwriting an existing record, we don't have to lock.
     追加--需加锁，覆盖--无需加锁
	 */
    if (SEEK_END == whence)		/* we're appending */
    {
        if (writew_lock(db->idxfd, ((db->nhash + 1)*PTR_SZ) + 1, \
            SEEK_SET, 0) < 0)
        {
            err_dump("_db_writeidx: writew_lock error");
        }
    }


	/*
	 * Position the index file and record the offset.
	 */
    if (-1 == (db->idxoff = lseek(db->idxfd, offset, whence)))
    {
        err_dump("_db_writeidx: lseek error");
    }

	iov[0].iov_base = asciiptrlen;
	iov[0].iov_len  = PTR_SZ + IDXLEN_SZ;
	iov[1].iov_base = db->idxbuf;
	iov[1].iov_len  = len;
    if (writev(db->idxfd, &iov[0], 2) != PTR_SZ + IDXLEN_SZ + len)
    {
        err_dump("_db_writeidx: writev error of index record");
    }

    if (SEEK_END == whence)
    {
        if (un_lock(db->idxfd, ((db->nhash + 1)*PTR_SZ) + 1, \
            SEEK_SET, 0) < 0)
        {
            err_dump("_db_writeidx: un_lock error");
        }
    }

}

/*
 * Write a chain ptr field somewhere in the index file:
 * the free list, the hash table, or in an index record.
 将一个链表指针写至索引文件
 */
static void _db_writeptr(DB *db, off_t offset, off_t ptrval)
{
	char	asciiptr[PTR_SZ + 1];

    memset(asciiptr, 0, sizeof(asciiptr));

    /*验证该指针在索引文件边界范围内*/
    if (ptrval < 0 || ptrval > PTR_MAX)
    {
        err_quit("_db_writeptr: invalid ptr: %d", ptrval);
    }
    /*变换成ascii字符串*/
	sprintf(asciiptr, "%*ld", PTR_SZ, ptrval);

    if (lseek(db->idxfd, offset, SEEK_SET) == -1)
    {
        err_dump("_db_writeptr: lseek error to ptr field");
    }

    if (write(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
    {
        err_dump("_db_writeptr: write error of ptr field");
    }
}

/*
 * Store a record in the database.  Return 0 if OK, 1 if record
 * exists and DB_INSERT specified, -1 on error.
 将一条记录加到数据库中
 */
int db_store(DBHANDLE h, const char *key, const char *data, int flag)
{
	DB		*db = h;
	int		rc = 0, keylen = 0, datlen = 0;
	off_t	ptrval;

	if ((flag != DB_INSERT) && (flag != DB_REPLACE) && \
	  (flag != DB_STORE)) 
    {
		errno = EINVAL;
		return(-1);
	}

	keylen = strlen(key);
	datlen = strlen(data) + 1;		/* +1 for newline at end */

    if ((datlen < DATLEN_MIN) || (datlen > DATLEN_MAX))
    {
        err_dump("db_store: invalid data length");
    }

	/*
	 * _db_find_and_lock calculates which hash table this new record
	 * goes into (db->chainoff), regardless of whether it already
	 * exists or not. The following calls to _db_writeptr change the
	 * hash table entry for this chain to point to the new record.
	 * The new record is added to the front of the hash chain.
     db_store很可能会修改散列链，所以需要加锁
	 */
	if (_db_find_and_lock(db, key, 1) < 0) 
    { 
        /* record not found */
		if (DB_REPLACE == flag) 
        {
			rc = -1;
			db->cnt_storerr++;
			errno = ENOENT;		/* error, record does not exist */
			goto doReturn;
		}

		/*
		 * _db_find_and_lock locked the hash chain for us; read
		 * the chain ptr to the first index record on hash chain.
		 */
		ptrval = _db_readptr(db, db->chainoff);

		if (_db_findfree(db, keylen, datlen) < 0) 
        {
			/*
			 * Can't find an empty record big enough. Append the
			 * new record to the ends of the index and data files.
			 */
			_db_writedat(db, data, 0, SEEK_END);
			_db_writeidx(db, key, 0, SEEK_END, ptrval);

			/*
			 * db->idxoff was set by _db_writeidx.  The new
			 * record goes to the front of the hash chain.
			 */
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor1++;
		} 
        else 
        {
			/*
			 * Reuse an empty record. _db_findfree removed it from
			 * the free list and set both db->datoff and db->idxoff.
			 * Reused record goes to the front of the hash chain.
			 */
			_db_writedat(db, data, db->datoff, SEEK_SET);
			_db_writeidx(db, key, db->idxoff, SEEK_SET, ptrval);
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor2++;
		}
	} 
    else 
    {						
        /* record found */
		if (DB_INSERT == flag) 
        {
			rc = 1;		/* error, record already in db */
			db->cnt_storerr++;
			goto doReturn;
		}

		/*
		 * We are replacing an existing record.  We know the new
		 * key equals the existing key, but we need to check if
		 * the data records are the same size.
		 */
		if (datlen != db->datlen) 
        {
            /* delete the existing record */
			_db_dodelete(db);	

			/*
			 * Reread the chain ptr in the hash table
			 * (it may change with the deletion).
			 */
			ptrval = _db_readptr(db, db->chainoff);

			/*
			 * Append new index and data records to end of files.
			 */
			_db_writedat(db, data, 0, SEEK_END);
			_db_writeidx(db, key, 0, SEEK_END, ptrval);

			/*
			 * New record goes to the front of the hash chain.
			 */
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor3++;
		} 
        else 
        {
			/*
			 * Same size data, just replace data record.
			 */
			_db_writedat(db, data, db->datoff, SEEK_SET);
			db->cnt_stor4++;
		}
	}

	rc = 0;		/* OK */

doReturn:	
    /* unlock hash chain locked by _db_find_and_lock */
    if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
    {
        err_dump("db_store: un_lock error");
    }

	return(rc);
}

/*
 * Try to find a free index record and accompanying data record
 * of the correct sizes.  We're only called by db_store.
 找到指定大小的空闲索引记录和相关联的数据记录
 */
static int _db_findfree(DB *db, int keylen, int datlen)
{
	int		rc = 0;
	off_t	offset, nextoffset, saveoffset;

	/*
	 * Lock the free list.
	 */
    if (writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("_db_findfree: writew_lock error");
    }

	/*
	 * Read the free list pointer.
	 */
	saveoffset = FREE_OFF;
	offset = _db_readptr(db, saveoffset);
    /*简单实现：只有当一个已删除记录的键长度及数据长度与要加入的键长度及数据长度一样时，
    才重用已删除的空间。其他更好的重用删除空间的方法一般更复杂。*/
	while (offset != 0) 
    {
		nextoffset = _db_readidx(db, offset);
        if ((strlen(db->idxbuf) == keylen) && (db->datlen == datlen))
        {
            break;		/* found a match */
        }
		saveoffset = offset;
		offset = nextoffset;
	}

	if (0 == offset) 
    {
		rc = -1;	/* no match found */
	} 
    else 
    {
		/*
		 * Found a free record with matching sizes.
		 * The index record was read in by _db_readidx above,
		 * which sets db->ptrval.  Also, saveoffset points to
		 * the chain ptr that pointed to this empty record on
		 * the free list.  We set this chain ptr to db->ptrval,
		 * which removes the empty record from the free list.
         将已找到记录的下一个链表指针写至前一记录的链表指针，从空闲链表中移除该记录
		 */
		_db_writeptr(db, saveoffset, db->ptrval);
		rc = 0;

		/*
		 * Notice also that _db_readidx set both db->idxoff
		 * and db->datoff.  This is used by the caller, db_store,
		 * to write the new index record and data record.
		 */
	}

	/*
	 * Unlock the free list.
	 */
    if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("_db_findfree: un_lock error");
    }

	return(rc);
}

/*
 * Rewind the index file for db_nextrec.
 * Automatically called by db_open.
 * Must be called before first db_nextrec.
 倒带，转回，回收数据库资源，把数据库重置到初始状态
 */
void db_rewind(DBHANDLE h)
{
	DB		*db = h;
	off_t	offset;

	offset = (db->nhash + 1) * PTR_SZ;	/* +1 for free list ptr */

	/*
	 * We're just setting the file offset for this process
	 * to the start of the index records; no need to lock.
	 * +1 below for newline at end of hash table.
	 */
    if ((db->idxoff = lseek(db->idxfd, offset + 1, SEEK_SET)) == -1)
    {
        err_dump("db_rewind: lseek error");
    }
}

/*
 * Return the next sequential record.
 * We just step our way through the index file, ignoring deleted
 * records.  db_rewind must be called before this function is
 * called the first time.
 1.返回数据库的下一条记录，返回值是指向数据缓冲的指针，并将键复制到非空的键指针指向的内存
 2.记录按他们在数据路文件中存放的顺序逐一返回，并不按键值大小排序
 3.并不跟随散列链表，可能读到已删除的记录，只是不向调用者返回这种及删除记录
 4.返回的是在变化的数据库中在某一时间的快照
 */
char *db_nextrec(DBHANDLE h, char *key)
{
	DB		*db = h;
	char	c;
	char	*ptr = NULL;

	/*
	 * We read lock the free list so that we don't read
	 * a record in the middle of its being deleted.
	 */
    if (readw_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("db_nextrec: readw_lock error");
    }

    /* loop until a nonblank key is found 逐条顺序读索引文件*/
	do {

		/* Read next sequential index record.*/
		if (_db_readidx(db, 0) < 0) 
        {
			ptr = NULL;		/* end of index file, EOF */
			goto doreturn;
		}

		/* Check if key is all blank (empty record)跳过全是空格的记录，不返回*/
		ptr = db->idxbuf;
        while ((c = *ptr++) != 0 && c == SPACE)
        {
            ;	/* skip until null byte or nonblank */
        }

	}while (c == 0);	

    if (key != NULL)
    {
        strcpy(key, db->idxbuf);	/* return key */
    }

	ptr = _db_readdat(db);	/* return pointer to data buffer */
	db->cnt_nextrec++;

doreturn:
    if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
    {
        err_dump("db_nextrec: un_lock error");
    }

	return(ptr);
}
/*
1.为简化起见，将所有函数都放在一个文件中，这样处理的有点是，只要将私有函数声明为static，就可对外将其隐藏起来；
2.锁
2.1.db_writeidx  对索引文件加锁，加锁范围从散列链末尾到文件末尾，不影响其他数据库的读写用户；；
2.2._db_writedat 对整个数据文件加锁,同样不影响其他数据库的读写用户；
2.3.读写数据库，对散列链加锁，不对数据文件加锁；
3.运行时动态库问题：
apue.2e/db$ ./t4
./t4: error while loading shared libraries: libapue_db.so.1: cannot open shared object file: No such file or directory

libapue_db.so.1 拷贝的/lib下面即可
*/
