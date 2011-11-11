#ifndef _DSK31_H_
#define _DSK31_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define CONNECT_LEN 4

bool dskConnect(int port);                             // 与前级建立连接
void dskDisconnect(void);                              // 断开与前级的连接
bool SendDskCmd(const char *CmdName, int count, ... ); // 执行命令

#endif
