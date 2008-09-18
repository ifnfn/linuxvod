#ifndef _SERIAL_H
#define _SERIAL_H

#include <fcntl.h>              /* open */
#include <sys/types.h>
#include <stdbool.h>

#ifdef LINUX
  #include <sys/signal.h>
  #include <termios.h>            /* tcgetattr, tcsetattr */
#else
  #include <windows.h>
#endif

int OpenComm(int port, int baudrate, int databit, const char *stopbit, char parity);
int ReadComm(int port, void *data, int datalength);
int SyncReadComm(int port, void *data, int datalangth);


/* 等待串口收到由data,len指定的数据
 */
bool WaitDataRight(int port, const unsigned char *data, int len);
int WriteComm(int port, const unsigned char * data, int datalength);
int CloseComm(int port);

#endif
/* serial.c */
