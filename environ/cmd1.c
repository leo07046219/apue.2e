/*7-4 进行命令处理的典型程序骨架
读命令，确定命令类型，调用相应函数处理每一条命令*/

#include "apue.h"

#define	TOK_ADD	   5

char *tok_ptr;		/* global pointer for get_token() */

void	do_line(char *);
void	cmd_add(void);
int		get_token(void);

int main(void)
{
	char	line[MAXLINE];

    memset(line, 0, sizeof(line));

    while (fgets(line, MAXLINE, stdin) != NULL)
    {
        do_line(line);
    }

	exit(0);
}

void do_line(char *ptr)		/* process one line of input */
{
	int		cmd = 0;

	tok_ptr = ptr;
	while ((cmd = get_token()) > 0) 
    {
		switch (cmd) 
        {	
            /* one case for each command */
		case TOK_ADD:
				cmd_add();
				break;
		}
	}
}

void cmd_add(void)
{
	int		token = 0;

	token = get_token();
	/* rest of processing for this command */
}

int get_token(void)
{
	/* fetch next token from line pointed to by tok_ptr */
}
