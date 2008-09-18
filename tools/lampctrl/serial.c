#include <stdio.h>              /* perror, printf, puts, fprintf, fputs */
#include <unistd.h>             /* read, write, close */
#include <string.h>             /* bzero, memcpy */
#include <limits.h>             /* CHAR_MAX */
#include <stdbool.h>

#include "serial.h"
#include "dsk31.h"

#define TIMEOUT_SEC(buflen,baud) (buflen*20/baud+2)
#define TIMEOUT_USEC 0

#define FALSE  -1
#define TRUE   0

static int BAUDRATE(int baudrate)
{
	switch (baudrate) {
		case 0     : return  B0     ;
		case 50    : return  B50    ;
		case 75    : return  B75    ;
		case 110   : return  B110   ;
		case 134   : return  B134   ;
		case 150   : return  B150   ;
		case 200   : return  B200   ;
		case 300   : return  B300   ;
		case 600   : return  B600   ;
		case 1200  : return  B1200  ;
		case 2400  : return  B2400  ;
		case 9600  : return  B9600  ;
		case 19200 : return  B19200 ;
		case 38400 : return  B38400 ;
		case 57600 : return  B57600 ;
		case 115200: return  B115200;
		default    : return  B9600  ;
	}
}

static int _BAUDRATE(int baudrate)
{
	switch (baudrate) {
		case B0     : return (0     );
		case B50    : return (50    );
		case B75    : return (75    );
		case B110   : return (110   );
		case B134   : return (134   );
		case B150   : return (150   );
		case B200   : return (200   );
		case B300   : return (300   );
		case B600   : return (600   );
		case B1200  : return (1200  );
		case B2400  : return (2400  );
		case B9600  : return (9600  );
		case B19200 : return (19200 );
		case B38400 : return (38400 );
		case B57600 : return (57600 );
		case B115200: return (115200);
		default     : return (9600  );
	}
}

static int GetBaudrate(int port)
{
	struct termios Opt;
	tcgetattr(port, &Opt);
	return _BAUDRATE(cfgetospeed(&Opt));
}

/*
 * @brief   设置串口通信速率
 * @param   fd     类型 int 打开串口的文件句柄
 * @param   speed  类型 int 串口速度
 * @return  void
*/
static void set_Speed(int fd, int speed)
{
	int i;
	int status;
	int sp = BAUDRATE(speed);
	struct termios Opt;
	tcgetattr(fd, &Opt);

	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&Opt, sp);
	cfsetospeed(&Opt, sp);
	status = tcsetattr(fd, TCSANOW, &Opt);
	if  (status != 0) {
		perror("tcsetattr fd1");
		return;
	}
	tcflush(fd,TCIOFLUSH);
}

/**
 * @brief  设置串口数据位，停止位和效验位
 * @param  fd       类型  int 打开的串口文件句柄
 * @param  databits 类型  int 数据位   取值为 7 或者8
 * @param  stopbits 类型  int 停止位   取值为 1 或者2
 * @param  parity   类型  int 效验类型 取值为 N,E,O,,S
*/
static int set_Parity(int fd, int databits, const char *stopbits, int parity)
{
	struct termios options;
	if  (tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");
		return FALSE;
	}
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   /*Output*/
	options.c_cflag &= ~CSIZE;
	switch (databits) {/*设置数据位数*/
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n"); return (FALSE);
	}
	switch (parity) {
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable    */
			options.c_iflag &= ~INPCK;    /* Enable parity checking */
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;    /* 转换为偶效验    */
			options.c_iflag |= INPCK;      /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
		    options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return FALSE;
	}
	if (0 == strcmp (stopbits, "1")) /* stop bit*/
		options.c_cflag &= ~CSTOPB;
	else if (0 == strcmp (stopbits, "1.5"))
		options.c_cflag &= ~CSTOPB;
	else if (0 == strcmp (stopbits, "2"))
		options.c_cflag |= CSTOPB;
	else
		options.c_cflag &= ~CSTOPB;

	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 150; /* 设置超时15 seconds*/
	options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0) {
		perror("SetupSerial 3");
		return FALSE;
	}
	return TRUE;
}

/**
 * @brief  打开端口
 * @param  ComPort  类型  int 端口号 0 ttyS0 1 ttyS1
 * @param  baudrate 类型  int 端口速度
 * @param  databits 类型  int 数据位   取值为 7 或者8
 * @param  stopbits 类型  const char * 停止位   取值为 1 或者2
 * @param  parity   类型  int 效验类型 取值为 N,E,O,,S
*/
int OpenComm(int ComPort, int baudrate, int databit, const char *stopbit, char parity)
{
	char pComPort[20];
	int retval, fd;
#ifdef LINUX
	sprintf(pComPort, "/dev/ttyS%d", ComPort);
	fd = open(pComPort, O_RDWR);         //| O_NOCTTY | O_NDELAY
	if (-1 == fd) {
		perror("Can't Open Serial Port");
		return -1;
	}
	set_Speed(fd, baudrate);
	if (set_Parity(fd, databit, stopbit, parity) == FALSE)  {
		printf("Set Parity Error\n");
		exit(0);
	}
	return fd;
#endif
}

int ReadComm(int port, void *data, int datalength)
{
	int retval = 0;
#ifdef LINUX
	struct timeval tv_timeout;
	fd_set fs_read;
	FD_ZERO (&fs_read);
	FD_SET (port, &fs_read);
	tv_timeout.tv_sec  = TIMEOUT_SEC(datalength, GetBaudrate(port));
	tv_timeout.tv_usec = TIMEOUT_USEC;
	retval = select(port + 1, &fs_read, NULL, NULL, &tv_timeout);
	if (retval)
		return read(port, data, datalength);
	else {
		return -1;
	}
#else
#endif
}

int SyncReadComm(int port, void *data, int datalangth)
{
	return read(port, data, datalangth);
//        int retval = 0;
#ifdef LINUX
//        struct timeval tv_timeout;
//        fd_set fs_read;
//        FD_ZERO (&fs_read);
//        FD_SET (port, &fs_read);
//        tv_timeout.tv_sec  = TIMEOUT_SEC(datalength, GetBaudrate(port));
//        tv_timeout.tv_usec = TIMEOUT_USEC;
//        retval = select(port + 1, &fs_read, NULL, NULL, &tv_timeout);
//        if (retval)
//                return read(port, data, datalength);
//        else {
//                return -1;
//        }
#else
#endif
}

bool WaitDataRight(int port, const unsigned char *data, int len)
{
	int i;
	unsigned char buf[101];
	memset(buf, 0, len + 1);
	printf("Wait data: ");
	for (i=0; i<len;i++)
		printf("%02x ", data[i]);
	int l = ReadComm(port, buf, 100);

	printf(" Get buf(%d): ", l);
	for (i=0; i<l;i++)
		printf("%02x ", buf[i]);
	printf("\n");
	if (l >= len) {
		for (i=l-len;i<l;i++)
			printf("%02X ", buf[i]);
		printf("\n");
		return memcmp(buf+l-len, data, len) == 0;
	}
	else
		return false;
}

/* 向串口发送数据
 * port       : 指定串口号
 * datalangth : 发送的数据长度
 * 返回实际已发送的数据长度, 阻塞模式
*/
int WriteComm(int port, const unsigned char *data, int datalength)
{
	int retval, len = 0, total_len = 0;
#ifdef LINUX
	struct timeval tv_timeout;
	fd_set fs_write;
	FD_ZERO(&fs_write);
	FD_SET(port, &fs_write);
	tv_timeout.tv_sec  = TIMEOUT_SEC(datalength, GetBaudrate(port));
	tv_timeout.tv_usec = TIMEOUT_USEC;
	for (total_len = 0, len = 0; total_len < datalength;) {
		retval = select(port + 1, NULL, &fs_write, NULL, &tv_timeout);
		if (retval) {
			len = write(port, &data[total_len], datalength - total_len);
			if (len > 0)
				total_len += len;
		}
		else {
			tcflush (port, TCOFLUSH);     /* flush all output data */
			break;
		}
	}
	return total_len;
#else
	DWORD ByteSend;
	return WriteFile(port.fd, data, datalength, &ByteSend, NULL); //重叠结构
#endif
}

int CloseComm(int port)
{
#ifdef LINUX
	tcflush(port, TCIFLUSH);
	close(port);
#else
	CloseHandle(port);
#endif
}
