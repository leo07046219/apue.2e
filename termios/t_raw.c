/*18-11 测试原始终端模式和cbreak终端模式*/

#include "apue.h"

/*信号处理函数，恢复串口配置，以免程序结束后残留串口修改*/
static void sig_catch(int signo)
{
	printf("signal caught\n");
	tty_reset(STDIN_FILENO);
	exit(0);
}

int main(void)
{
	int		i = 0;
	char	c = 0;

    if (signal(SIGINT, sig_catch) == SIG_ERR)	/* catch signals */
    {
        err_sys("signal(SIGINT) error");
    }
    if (signal(SIGQUIT, sig_catch) == SIG_ERR)
    {
        err_sys("signal(SIGQUIT) error");
    }
    if (signal(SIGTERM, sig_catch) == SIG_ERR)
    {
        err_sys("signal(SIGTERM) error");
    }

    /*原始模式测试*/
    if (tty_raw(STDIN_FILENO) < 0)
    {
        err_sys("tty_raw error");
    }

	printf("Enter raw mode characters, terminate with DELETE\n");

    /*和键盘协议有关系，dell5804键盘delete发的不是177，无法停止*/
	while (1 == (i = read(STDIN_FILENO, &c, 1))) 
    {
        /* 0177 = ASCII DELETE */
        if ((c &= 255) == 133)		
        {
            break;
        }
		printf("%o\n", c);
	}

    if (tty_reset(STDIN_FILENO) < 0)
    {
        err_sys("tty_reset error");
    }
    if (i <= 0)
    {
        err_sys("read error");
    }

    /*cbreak模式测试*/
    if (tty_cbreak(STDIN_FILENO) < 0)
    {
        err_sys("tty_cbreak error");
    }

	printf("\nEnter cbreak mode characters, terminate with SIGINT\n");

	while ((i = read(STDIN_FILENO, &c, 1)) == 1) 
    {
		c &= 255;
		printf("%o\n", c);
	}
    if (tty_reset(STDIN_FILENO) < 0)
    {
        err_sys("tty_reset error");
    }
    if (i <= 0)
    {
        err_sys("read error");
    }

	exit(0);
}
